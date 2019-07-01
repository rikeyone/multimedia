#include <stdio.h>  

#include <stdlib.h>  

#include <string.h>  

#include <assert.h>  

#include <getopt.h>               

#include <fcntl.h>                

#include <unistd.h>  

#include <errno.h>  

#include <malloc.h>  

#include <sys/stat.h>  

#include <sys/types.h>  

#include <sys/time.h>  

#include <sys/mman.h>  

#include <sys/ioctl.h>  

#include <asm/types.h>            

#include <linux/videodev2.h>  

#define CLEAR(x) memset (&(x), 0, sizeof (x))  

typedef enum {  

    IO_METHOD_READ, IO_METHOD_MMAP, IO_METHOD_USERPTR,  

} io_method;  

struct buffer {  

    void * start;  

    size_t length;//buffer's length is different from cap_image_size  

};  

static char * dev_name = NULL;  

static io_method io = IO_METHOD_MMAP;//IO_METHOD_READ;//IO_METHOD_MMAP;  

static int fd = -1;  

struct buffer * buffers = NULL;  

static unsigned int n_buffers = 0;  

static FILE * outf = 0;  

static unsigned int cap_image_size = 0;//to keep the real image size!!  

//////////////////////////////////////////  

static void errno_exit(const char * s) {  

    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));  

    exit(EXIT_FAILURE);  

}  

static int xioctl(int fd, int request, void * arg) {  

    int r;  

    do  

        r = ioctl(fd, request, arg);  

    while (-1 == r && EINTR == errno);  

    return r;  

}  

static void process_image(const void * p, int len) {  

    //  static char[115200] Outbuff ;  

    fputc('.', stdout);  

    if (len > 0) {  

        fputc('.', stdout);  

        fwrite(p, 1, len, outf);  

    }  

    fflush(stdout);  

}  

static int read_frame(void) {  

    struct v4l2_buffer buf;  

    unsigned int i;  

    switch (io) {  

    case IO_METHOD_READ:  
        break;  

    case IO_METHOD_MMAP:  

        CLEAR (buf);  

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  

        buf.memory = V4L2_MEMORY_MMAP;  

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {  

            switch (errno) {  

            case EAGAIN:  

                return 0;  

            case EIO:  

                  

                  

            default:  

                errno_exit("VIDIOC_DQBUF");  

            }  

        }  

        assert(buf.index < n_buffers);  


        printf("image_size = %d,\t IO_METHOD_MMAP buffer.length=%d\r",  

                cap_image_size, buffers[buf.index].length);  

        process_image(buffers[buf.index].start, cap_image_size);  

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))  

            errno_exit("VIDIOC_QBUF");  

        break;  

    case IO_METHOD_USERPTR:  

        CLEAR (buf);  

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  

        buf.memory = V4L2_MEMORY_USERPTR;  

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {  

            switch (errno) {  

            case EAGAIN:  

                return 0;  

            case EIO:  

                  

                  

            default:  

                errno_exit("VIDIOC_DQBUF");  

            }  

        }  

        for (i = 0; i < n_buffers; ++i)  

            if (buf.m.userptr == (unsigned long) buffers[i].start && buf.length  

                    == buffers[i].length)  

                break;  

        assert(i < n_buffers);  

        printf("image_size = %d,\t IO_METHOD_USERPTR buffer.length=%d\r",  

                cap_image_size,  buffers[buf.index].length); 

        process_image(buffers[buf.index].start, cap_image_size);   

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))  

            errno_exit("VIDIOC_QBUF");  

        break;  

    }  

    return 1;  

}  

static void mainloop(void) {  

    unsigned int count;  
	
    count = 1000;  
	
	printf("start mainloop\n");
    while (count-- > 0) {  
		printf("count=%d\n",count);
        for (;;) {  

            fd_set fds;  

            struct timeval tv;  

            int r;  

            FD_ZERO(&fds);  

            FD_SET(fd, &fds);  

              

            tv.tv_sec = 2;  

            tv.tv_usec = 0;  

            r = select(fd + 1, &fds, NULL, NULL, &tv);  

            if (-1 == r) {  

                if (EINTR == errno)  

                    continue;  

                errno_exit("select");  

            }  

            if (0 == r) {  

                fprintf(stderr, "select timeout\n");  

                exit(EXIT_FAILURE);  

            }  

            if (read_frame())  

                break;  

              

        }  

    }  

}  

static void stop_capturing(void) {  

    enum v4l2_buf_type type;  

    switch (io) {  

    case IO_METHOD_READ:  

          

        break;  

    case IO_METHOD_MMAP:  

    case IO_METHOD_USERPTR:  

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  

        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))  

            errno_exit("VIDIOC_STREAMOFF");  

        break;  

    }  

}  

static void start_capturing(void) {  

    unsigned int i;  

    enum v4l2_buf_type type;  

    switch (io) {  

    case IO_METHOD_READ:  
        break;  

    case IO_METHOD_MMAP:  

        for (i = 0; i < n_buffers; ++i) {  

            struct v4l2_buffer buf;  

            CLEAR (buf);  

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  

            buf.memory = V4L2_MEMORY_MMAP;  

            buf.index = i;  

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))  

                errno_exit("VIDIOC_QBUF");  

        }  

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  

        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))  

            errno_exit("VIDIOC_STREAMON");  

        break;  

    case IO_METHOD_USERPTR:  

        for (i = 0; i < n_buffers; ++i) {  

            struct v4l2_buffer buf;  

            CLEAR (buf);  

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  

            buf.memory = V4L2_MEMORY_USERPTR;  

            buf.index = i;  

            buf.m.userptr = (unsigned long) buffers[i].start;  

            buf.length = buffers[i].length;  

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))  

                errno_exit("VIDIOC_QBUF");  

        }  

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  

        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))  

            errno_exit("VIDIOC_STREAMON");  

        break;  

    }  

}  

static void uninit_device(void) {  

    unsigned int i;  

    switch (io) {  

    case IO_METHOD_READ:  

        free(buffers[0].start);  

        break;  

    case IO_METHOD_MMAP:  

        for (i = 0; i < n_buffers; ++i)  

            if (-1 == munmap(buffers[i].start, buffers[i].length))  

                errno_exit("munmap");  

        break;  

    case IO_METHOD_USERPTR:  

        for (i = 0; i < n_buffers; ++i)  

            free(buffers[i].start);  

        break;  

    }  

    free(buffers);  

}  

static void init_read(unsigned int buffer_size) {  

    buffers = calloc(1, sizeof(*buffers));  

    if (!buffers) {  

        fprintf(stderr, "Out of memory\n");  

        exit(EXIT_FAILURE);  

    }  

    buffers[0].length = buffer_size;  

    buffers[0].start = malloc(buffer_size);  

    if (!buffers[0].start) {  

        fprintf(stderr, "Out of memory\n");  

        exit(EXIT_FAILURE);  

    }  

}  

static void init_mmap(void) {  

    struct v4l2_requestbuffers req;  

    CLEAR (req);  

    req.count = 4;  

    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  

    req.memory = V4L2_MEMORY_MMAP;  

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {  

        if (EINVAL == errno) {  

            fprintf(stderr, "%s does not support "  

                "memory mapping\n", dev_name);  

            exit(EXIT_FAILURE);  

        } else {  

            errno_exit("VIDIOC_REQBUFS");  

        }  

    }  

    if (req.count < 2) {  

        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);  

        exit(EXIT_FAILURE);  

    }  

    buffers = calloc(req.count, sizeof(*buffers));  

    if (!buffers) {  

        fprintf(stderr, "Out of memory\n");  

        exit(EXIT_FAILURE);  

    }  

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {  

        struct v4l2_buffer buf;  

        CLEAR (buf);  

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  

        buf.memory = V4L2_MEMORY_MMAP;  

        buf.index = n_buffers;  

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))  

            errno_exit("VIDIOC_QUERYBUF");  

        buffers[n_buffers].length = buf.length;  

        buffers[n_buffers].start = mmap(NULL , buf.length,  

                PROT_READ | PROT_WRITE ,  

                MAP_SHARED , fd, buf.m.offset);  

        if (MAP_FAILED == buffers[n_buffers].start)  

            errno_exit("mmap");  

    }  

}  

static void init_userp(unsigned int buffer_size) {  

    struct v4l2_requestbuffers req;  

    unsigned int page_size;  

    page_size = getpagesize();  

    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);  

    CLEAR (req);  

    req.count = 4;  

    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  

    req.memory = V4L2_MEMORY_USERPTR;  

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {  

        if (EINVAL == errno) {  

            fprintf(stderr, "%s does not support "  

                "user pointer i/o\n", dev_name);  

            exit(EXIT_FAILURE);  

        } else {  

            errno_exit("VIDIOC_REQBUFS");  

        }  

    }  

    buffers = calloc(4, sizeof(*buffers));  

    if (!buffers) {  

        fprintf(stderr, "Out of memory\n");  

        exit(EXIT_FAILURE);  

    }  

    for (n_buffers = 0; n_buffers < 4; ++n_buffers) {  

        buffers[n_buffers].length = buffer_size;  

        buffers[n_buffers].start = memalign(page_size,  

                buffer_size);  

        if (!buffers[n_buffers].start) {  

            fprintf(stderr, "Out of memory\n");  

            exit(EXIT_FAILURE);  

        }  

    }  

}  

static void init_device(void) {  

    struct v4l2_capability cap;  

    struct v4l2_cropcap cropcap;  

    struct v4l2_crop crop;  


    unsigned int min;  

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {  

        if (EINVAL == errno) {  

            fprintf(stderr, "%s is no V4L2 device\n", dev_name);  

            exit(EXIT_FAILURE);  

        } else {  

            errno_exit("VIDIOC_QUERYCAP");  

        }  

    }  

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {  

        fprintf(stderr, "%s is no video capture device\n", dev_name);  

        exit(EXIT_FAILURE);  

    }  

    switch (io) {  

    case IO_METHOD_READ:  

        if (!(cap.capabilities & V4L2_CAP_READWRITE)) {  

            fprintf(stderr, "%s does not support read i/o\n", dev_name);  

            exit(EXIT_FAILURE);  

        }  
        break;  

    case IO_METHOD_MMAP:  

    case IO_METHOD_USERPTR:  

        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {  

            fprintf(stderr, "%s does not support streaming i/o\n", dev_name);  

            exit(EXIT_FAILURE);  

        }  
        break;  

    }  

    //////not all capture support crop!!!!!!!  

    printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");  

    CLEAR (cropcap);  

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {  
        printf("----->has ability to crop!!\n");  
    } else {  
        printf("!! has no ability to crop!!\n");  
    }  
    printf("-#-#-#-#-#-#-#-#-#-#-#-#-#-\n");  

    cap_image_size = 15 * 1024;  

    switch (io) {  

    case IO_METHOD_READ:  
        break;  

    case IO_METHOD_MMAP:  

        init_mmap();  

        break;  

    case IO_METHOD_USERPTR:  

        init_userp(cap_image_size);  

        break;  

    }  

}  

static void close_device(void) {  

    if (-1 == close(fd))  

        errno_exit("close");  

    fd = -1;  

}  

static void open_device(void) {  

    struct stat st;  

    if (-1 == stat(dev_name, &st)) {  

        fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno,  

                strerror(errno));  

        exit(EXIT_FAILURE);  

    }  

    if (!S_ISCHR(st.st_mode)) {  

        fprintf(stderr, "%s is no device\n", dev_name);  

        exit(EXIT_FAILURE);  

    }  

    fd = open(dev_name, O_RDWR, 0);  

    if (-1 == fd) {  

        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno,  

                strerror(errno));  

        exit(EXIT_FAILURE);  

    }  

}  

static void usage(FILE * fp, int argc, char ** argv) {  

    fprintf(fp, "Usage: %s [options]\n\n"  

        "Options:\n"  

        "-d | --device name   Video device name [/dev/video0]\n"  

        "-h | --help          Print this message\n"  

        "-m | --mmap          Use memory mapped buffers\n"  

        "-r | --read          Use read() calls\n"  

        "-u | --userp         Use application allocated buffers\n"  

        "", argv[0]);  

}  

static const char short_options[] = "d:hmru";  

static const struct option long_options[] = { { "device", required_argument,  

        NULL, 'd' }, { "help", no_argument, NULL, 'h' }, { "mmap", no_argument,  

        NULL, 'm' }, { "read", no_argument, NULL, 'r' }, { "userp",  

        no_argument, NULL, 'u' }, { 0, 0, 0, 0 } };  
		
		
static void print_qctrl(struct v4l2_queryctrl *qctrl)
{
  struct v4l2_control ctrl;

  ctrl.id = qctrl->id;
  if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) < 0) {
    perror("get ctrl failed");
    ctrl.value = -999;
  }

  printf("%-14s : id=%08x, type=%d, minimum=%d, maximum=%d\n"
         "\t\t value = %d, step=%d, default_value=%d\n", 
         qctrl->name, qctrl->id, qctrl->type, qctrl->minimum, qctrl->maximum,
         ctrl.value, qctrl->step, qctrl->default_value);
}

static int set_qctrl(struct v4l2_queryctrl *qctrl, int value)
{
  struct v4l2_control ctrl;

  printf("set freq = %d\n", value);

  ctrl.id = qctrl->id;
  ctrl.value = value;
  return ioctl(fd, VIDIOC_S_CTRL, &ctrl);
}	
	
void set_freq_control(int freq)
{
	struct v4l2_queryctrl qctrl;
	memset(&qctrl, 0, sizeof(qctrl));
	qctrl.id = V4L2_CID_PRIVATE_BASE;
    if (ioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == 0) {
      print_qctrl(&qctrl);
      set_qctrl(&qctrl, freq);
	  print_qctrl(&qctrl);
    }
}
int main(int argc, char ** argv) {  

    dev_name = "/dev/video4";  

    outf = fopen("/data/dtmb_capture.ts", "wb");  

    for (;;) {  

        int index;  

        int c;  

        c = getopt_long(argc, argv, short_options, long_options, &index);  

        if (-1 == c)  

            break;  

        switch (c) {  

        case 0:   

            break;  

        case 'd':  

            dev_name = optarg;  

            break;  

        case 'h':  

            usage(stdout, argc, argv);  

            exit(EXIT_SUCCESS);  

        case 'm':  

            io = IO_METHOD_MMAP;  

            break;  

        case 'r':  

            io = IO_METHOD_READ;  

            break;  

        case 'u':  

            io = IO_METHOD_USERPTR;  

            break;  

        default:  

            usage(stderr, argc, argv);  

            exit(EXIT_FAILURE);  

        }  

    }  

    open_device();  
    init_device();  
	set_freq_control(698000000);
    start_capturing();  
    mainloop();  
    stop_capturing();  
    fclose(outf);  
    uninit_device();  
    close_device();  
    return 0;  
}
