// SPDX-License-Identifier: GPL-3.0
/*
 * Copyright (C) 2022 sokiAdachi.  All rights reserved.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>

MODULE_AUTHOR("Soki Adachi"); //作者
MODULE_DESCRIPTION("driver for LED control"); //何のモジュールか
MODULE_LICENSE("GPL"); //ライセンス
MODULE_VERSION("0.0.1"); //バージョン

static dev_t dev;
static struct cdev cdv; //キャラクタデバイスの情報を格納する構造体
static struct class *cls = NULL;

static volatile u32 *gpio_base = NULL; //アドレスをマッピングするための配列をグローバルで定義

static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos){
        char c; //読み込んだ字を入れる変数
        if(copy_from_user(&c, buf, sizeof(char))) //カーネルの外からの字を取り込む
        return -EFAULT;

        //printk(KERN_INFO "led_write is called\n");
        printk(KERN_INFO "receive %c\n",c);

	if(c == '0'){
		gpio_base[10] = 1 << 23; //LED1を消す
	}else if(c == '1'){
		gpio_base[7] = 1 << 23; //LED1をつける
	}else if(c == '2'){
		gpio_base[10] = 1 << 24; //LED2を消す
	}else if(c == '3'){
		gpio_base[7] = 1 << 24; //LED2をつける
	}else if(c == '4'){
		gpio_base[10] = 1 << 25; //
	}else if(c == '5'){
		gpio_base[7] = 1 << 25; //
	}

        return 1; //読み込んだ文字数を返す（この場合はダミーの1）
}
/*
static ssize_t sushi_read(struct file* filp, char* buf, size_t count, loff_t* pos){
        int size = 0;
        char sushi[] = {0xF0, 0x9F, 0x8D, 0xA3, 0x0A}; //寿司の絵文字のバイナリ
        if(copy_to_user(buf+size,(const char *)sushi, sizeof(sushi))){
                printk( KERN_INFO "sushi : copy_to_user failed\n" );
                return -EFAULT;
        }
        size += sizeof(sushi);
        return size;
}*/

static struct file_operations led_fops = { //挙動を書いた関数のポインタを格納する構造体
        .owner = THIS_MODULE,
        .write = led_write
//	.read = sushi_read
};

static int __init init_mod(void){ //カーネルモジュールの初期化
        int retval;
        retval = alloc_chrdev_region(&dev, 0, 1, "myled"); //デバイス番号の取得
        if(retval < 0){
                printk(KERN_ERR "alloc_chrdev_region failed. \n"); 
                return retval; 
        }
        printk(KERN_INFO "%s is loaded. major:%d/n",__FILE__,MAJOR(dev)); //ログを吐くように

        cdev_init(&cdv, &led_fops); //キャラクタデバイスの初期化
        retval = cdev_add(&cdv, dev, 1); //キャラクタデバイスをカーネルに登録
        if(retval < 0){
                printk(KERN_ERR "cdev_add failed. major:%d, minor:%d",MAJOR(dev),MINOR(dev));
                return retval;
        }
        cls = class_create(THIS_MODULE,"myled");
        if (IS_ERR(cls)){
                printk(KERN_ERR "class_create failed.");
                return PTR_ERR(cls);
        }
        device_create(cls, NULL, dev, NULL, "myled%d",MINOR(dev)); //デバイス情報の作成
        
	gpio_base = ioremap_nocache(0x3f200000, 0xA0); //GPIOのレジスタの最初のアドレス
	
	const u32 led1 = 23; //23番にLED1がある
	const u32 led2 = 24; //24番にLED2がある
	const u32 led3 = 25; 
	const u32 index1 = led1/10; //GPFSEL2
	const u32 index2 = led2/10; //GPFSEL2
	const u32 index3 = led3/10;
	const u32 shift1 = (led1%10)*3; //9bit
	const u32 shift2 = (led2%10)*3; //12bit
	const u32 shift3 = (led3%10)*3; 
	const u32 mask1 = ~(0x7 << shift1); //
	const u32 mask2 = ~(0x7 << shift2); //
	const u32 mask3 = ~(0x7 << shift3);
	gpio_base[index1] = (gpio_base[index1] & mask1) | (0x1 << shift1); //001:output flag
	gpio_base[index2] = (gpio_base[index2] & mask2) | (0x1 << shift2); //001:output flag
	gpio_base[index3] = (gpio_base[index3] & mask3) | (0x1 << shift3);
	
	return 0;
}

static void __exit cleanup_mod(void){ //後始末
        cdev_del(&cdv); //キャラクタデバイスの破棄
        device_destroy(cls, dev); //デバイス情報の削除
        class_destroy(cls); //クラスの削除
        unregister_chrdev_region(dev, 1); //デバイス番号の解放
        printk(KERN_INFO "%s is unloaded. major:%d\n",__FILE__,MAJOR(dev)); //ログを吐くように
}

module_init(init_mod);     // マクロで関数を登録
module_exit(cleanup_mod);  // 同上
