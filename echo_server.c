#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched/signal.h>
#include <linux/tcp.h>
#include <linux/types.h>

#include "echo_server.h"

#define BUF_SIZE 1024

struct runtime_statistics stats = {ATOMIC_INIT(0)};
struct echo_service daemon = {.is_stopped = false};
extern struct workqueue_struct *kecho_wq;
extern bool bench;

static int get_request(struct socket *sock, unsigned char *buf, size_t size)
{
    struct msghdr msg;
    struct kvec vec;
    int length;

    /* kvec setting */
    vec.iov_len = size;
    vec.iov_base = buf;

    /* msghdr setting */
    msg.msg_name = 0;
    msg.msg_namelen = 0;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    length = kernel_recvmsg(sock, &msg, &vec, size, size, msg.msg_flags);
    if (length)
        TRACE(recv_msg);

    return length;
}

static int send_request(struct socket *sock, unsigned char *buf, size_t size)
{
    int length;
    struct kvec vec;
    struct msghdr msg;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    vec.iov_base = buf;
    vec.iov_len = strlen(buf);

    length = kernel_sendmsg(sock, &msg, &vec, 1, size);
    TRACE(send_msg);

    return length;
}

static void echo_server_worker(struct work_struct *work)
{
    struct kecho *worker = container_of(work, struct kecho, kecho_work);
    unsigned char *buf;

    buf = kzalloc(BUF_SIZE, GFP_KERNEL);
    if (!buf) {
        TRACE(kmal_err);
        return;
    }

    while (!daemon.is_stopped) {
        int res = get_request(worker->sock, buf, BUF_SIZE - 1);
        if (res <= 0) {
            if (res)
                TRACE(recv_err);
            break;
        }

        res = send_request(worker->sock, buf, res);
        if (res < 0) {
            TRACE(send_err);
            break;
        }
        memset(buf, 0, res);
    }
    kernel_sock_shutdown(worker->sock, SHUT_RDWR);
    kfree(buf);
    TRACE(shdn_msg);
}

static struct work_struct *create_work(struct socket *sk)
{
    struct kecho *work;
    if (!(work = kmalloc(sizeof(struct kecho), GFP_KERNEL)))
        return NULL;
    work->sock = sk;
    INIT_WORK(&work->kecho_work, echo_server_worker);
    list_add(&work->list, &daemon.worker);
    return &work->kecho_work;
}

/* it would be better if we do this dynamically */
static void free_work(void)
{
    struct kecho *l, *tar;
    /* cppcheck-suppress uninitvar */

    list_for_each_entry_safe (tar, l, &daemon.worker, list) {
        kernel_sock_shutdown(tar->sock, SHUT_RDWR);
        flush_work(&tar->kecho_work);
        sock_release(tar->sock);
        kfree(tar);
    }
}

void do_analysis(void)
{
    smp_mb();
    TRACE_PRINT(KERN_ERR, recv_msg);
    TRACE_PRINT(KERN_ERR, send_msg);
    TRACE_PRINT(KERN_ERR, shdn_msg);
    TRACE_PRINT(KERN_ERR, recv_err);
    TRACE_PRINT(KERN_ERR, send_err);
    TRACE_PRINT(KERN_ERR, kmal_err);
    TRACE_PRINT(KERN_ERR, work_err);
    TRACE_PRINT(KERN_ERR, acpt_err);
}

int echo_server_daemon(void *arg)
{
    struct echo_server_param *param = arg;
    struct socket *sock;
    struct work_struct *work;

    allow_signal(SIGKILL);
    allow_signal(SIGTERM);

    INIT_LIST_HEAD(&daemon.worker);

    while (!kthread_should_stop()) {
        /* using blocking I/O */
        int error = kernel_accept(param->listen_sock, &sock, 0);
        if (error < 0) {
            if (signal_pending(current))
                break;
            // socket accept error
            TRACE(acpt_err);
            continue;
        }

        if (unlikely(!(work = create_work(sock)))) {
            printk(KERN_ERR MODULE_NAME
                   ": create work error, connection closed\n");
            TRACE(work_err);
            kernel_sock_shutdown(sock, SHUT_RDWR);
            sock_release(sock);
            continue;
        }
        queue_work(kecho_wq, work);
    }

    printk(MODULE_NAME ": daemon shutdown in progress...\n");
    do_analysis();
    daemon.is_stopped = true;
    free_work();

    return 0;
}
