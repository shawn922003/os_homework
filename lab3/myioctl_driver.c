#include <linux/module.h>   // Linux 模組相關的函式庫
#include <linux/fs.h>       // 檔案系統操作函式庫
#include <linux/uaccess.h>  // 用戶空間和內核空間資料訪問函式庫
#include <linux/ioctl.h>    // ioctl 系統呼叫相關函式庫
#include <linux/kernel.h>   // 核心函式庫
#include <linux/init.h>     // 模組初始化和退出相關函式
#include <linux/slab.h>     // 動態記憶體分配相關函式庫
#include <linux/file.h>     // 檔案操作相關函式庫

// 定義 ioctl 的 magic number
#define MYIOCTL_MAGIC 'k'

// 定義 ioctl 指令
#define MYIOCTL_RESET _IO(MYIOCTL_MAGIC, 0)           // 重置計數器的命令
#define MYIOCTL_GET_COUNT _IOR(MYIOCTL_MAGIC, 1, int) // 取得計數器值的命令
#define MYIOCTL_INCREMENT _IOW(MYIOCTL_MAGIC, 2, int) // 增加計數器值的命令

// 預設的計數值儲存檔案路徑
#ifndef PERSIST_FILE_PATH
#define PERSIST_FILE_PATH "/tmp/myioctl_count"
#endif

MODULE_LICENSE("GPL");              // 定義模組的許可協議
MODULE_AUTHOR("Group 1");           // 定義模組的作者
MODULE_DESCRIPTION("Simple IOCTL Example"); // 模組描述

static int myioctl_major; // 記錄裝置的主代號
static int count = 0;     // 計數器的初始值

static const char *persist_file = PERSIST_FILE_PATH; // 儲存計數器值的檔案路徑

// 函式宣告
static int myioctl_open(struct inode *inode, struct file *filp);
static int myioctl_release(struct inode *inode, struct file *filp);
static long myioctl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

// 定義檔案操作結構
static const struct file_operations myioctl_fops = {
    .open = myioctl_open,         // 開啟裝置時呼叫的函式
    .release = myioctl_release,  // 關閉裝置時呼叫的函式
    .unlocked_ioctl = myioctl_ioctl, // 處理 ioctl 命令的函式
};

// 將計數值儲存到檔案
static void save_count_to_file(void) {
    struct file *file;
    loff_t pos = 0;

    file = filp_open(persist_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); // 開啟檔案（寫入模式）
    if (IS_ERR(file)) {
        pr_err("Failed to open file for saving count\n"); // 開啟失敗時輸出錯誤訊息
        return;
    }

    if (kernel_write(file, &count, sizeof(count), &pos) < 0) { // 寫入計數器值
        pr_err("Failed to write count to file\n");
    }

    filp_close(file, NULL); // 關閉檔案
}

// 從檔案載入計數值
static void load_count_from_file(void) {
    struct file *file;
    loff_t pos = 0;
    int read_count = 0;

    file = filp_open(persist_file, O_RDONLY, 0); // 開啟檔案（讀取模式）
    if (IS_ERR(file)) {
        pr_info("No previous count found, starting from 0\n"); // 若檔案不存在，初始化計數器為 0
        count = 0;
        return;
    }

    if (kernel_read(file, &read_count, sizeof(read_count), &pos) > 0) { // 讀取計數器值
        count = read_count;
    } else {
        pr_info("Failed to read count from file, starting from 0\n"); // 若讀取失敗，初始化計數器為 0
        count = 0;
    }

    filp_close(file, NULL); // 關閉檔案
}

// 模組初始化函式
static int __init myioctl_init(void) {
    myioctl_major = register_chrdev(0, "myioctl", &myioctl_fops); // 註冊字符裝置

    if (myioctl_major < 0) {
        pr_err("Failed to register character device\n"); // 註冊失敗時輸出錯誤訊息
        return myioctl_major;
    }

    load_count_from_file(); // 從檔案載入計數值
    pr_info("myioctl device registered with major number %d\n", myioctl_major);
    pr_info("Current persist file: %s\n", persist_file);
    pr_info("Current counter value: %d\n", count);
    return 0;
}

// 模組退出函式
static void __exit myioctl_exit(void) {
    pr_info("myioctl device unloaded\n");
}

// 裝置開啟函式
static int myioctl_open(struct inode *inode, struct file *filp) {
    pr_info("myioctl device opened\n");
    return 0;
}

// 裝置關閉函式
static int myioctl_release(struct inode *inode, struct file *filp) {
    save_count_to_file(); // 儲存計數值到檔案
    pr_info("myioctl device closed\n");
    return 0;
}

// 處理 ioctl 命令的函式
static long myioctl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    int err = 0;
    int tmp;

    if (_IOC_TYPE(cmd) != MYIOCTL_MAGIC) { // 驗證 magic number
        pr_err("Invalid magic number\n");
        return -ENOTTY;
    }

    switch (cmd) {
        case MYIOCTL_RESET: // 重置計數器
            pr_info("IOCTL: Resetting counter\n");
            count = 0;
            break;

        case MYIOCTL_GET_COUNT: // 取得計數器值
            pr_info("IOCTL: Getting counter value\n");
            if (copy_to_user((int *)arg, &count, sizeof(int))) { // 將資料複製到用戶空間
                pr_err("Failed to copy data to user space\n");
                err = -EFAULT;
            }
            break;

        case MYIOCTL_INCREMENT: // 增加計數器值
            pr_info("IOCTL: Incrementing counter\n");
            if (copy_from_user(&tmp, (int *)arg, sizeof(int))) { // 從用戶空間複製資料
                pr_err("Failed to copy data from user space\n");
                err = -EFAULT;
            } else {
                count += tmp; // 更新計數器值
            }
            break;

        default: // 未知的 ioctl 指令
            pr_err("Unknown IOCTL command\n");
            return -ENOTTY;
    }

    return err;
}

module_init(myioctl_init); // 設定模組初始化函式
module_exit(myioctl_exit); // 設定模組退出函式
