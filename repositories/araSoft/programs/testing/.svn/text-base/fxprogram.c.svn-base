#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define ATRI_FX2_VID 0x04b4
#define ATRI_FX2_PID 0x9999

// Current point: x
// Max points: n
// Update period: r
// Bar width: w
void loadBar(int x, int n, int r, int w);

libusb_device_handle *devopen() {
  int ret;
  libusb_device_handle *dev;
  libusb_init(NULL);

  dev = libusb_open_device_with_vid_pid(NULL, ATRI_FX2_VID, ATRI_FX2_PID);
  if (dev == NULL) {
    fprintf(stderr, "%s : could not open device\n", __FUNCTION__);
    return NULL;
  }
  if ((ret = libusb_set_configuration(dev, 1)) < 0) {
    fprintf(stderr, "%s : could not set configuration (%d)\n", __FUNCTION__,
	    ret);
    return NULL;
  }
  if ((ret = libusb_claim_interface(dev, 0)) < 0) {
    fprintf(stderr, "%s : could not claim interface (%d)\n", __FUNCTION__,
	    ret);
    return NULL;
  }
  return dev;
}

void devclose(libusb_device_handle *dev) {
  libusb_release_interface(dev, 0);
  libusb_close(dev);
  libusb_exit(NULL);
}

#define VR_FPGA_SU 0xB1
#define VR_FPGA_XFER 0xB2
#define VR_FPGA_DONE 0xB3

int fx2fpgasu(libusb_device_handle *dev) {
  return libusb_control_transfer(dev,
				 LIBUSB_ENDPOINT_OUT,  // type
				 VR_FPGA_SU,           // request
				 0,                    // value
				 0,                    // index
				 NULL,                 // data
				 0,                    // dataLength
				 10000);               // timeout
}

int fx2fpgaprog(libusb_device_handle *dev, unsigned char *buf,
		ssize_t nb) {
  return libusb_control_transfer(dev,
				 LIBUSB_ENDPOINT_OUT,
				 VR_FPGA_XFER,
				 0,
				 0,
				 buf,
				 nb,
				 10000);
}

int fx2fpgadone(libusb_device_handle *dev) {
  return libusb_control_transfer(dev,
				 LIBUSB_ENDPOINT_OUT,
				 VR_FPGA_DONE,
				 0,
				 0,
				 NULL,
				 0,
				 10000);
}
				 

#define BUFSIZE 64

int main(int argc, char **argv) {
  off_t size;
  int fd;
  libusb_device_handle *dev;
  int ret;
  struct stat st;
  unsigned char buf[BUFSIZE];
  ssize_t nb;
  ssize_t cur;
  ssize_t max;

  if (argc < 1) {
    fprintf(stderr, "%s : need a filename to program\n", argv[0]);
    exit(1);
  }
  if ((fd = open(argv[1], O_RDONLY)) < 0) {
    fprintf(stderr, "%s : could not open file '%s'\n", argv[0], argv[1]);
    exit(1);
  }
  
  dev = devopen();
  if (!dev) {
    fprintf(stderr, "%s : could not open device\n", argv[0]);
    close(fd);
    exit(1);
  }
  if ((ret = fx2fpgasu(dev)) < 0) {
    fprintf(stderr, "%s : error %d setting up FPGA transfer\n", argv[0],
	    ret);
    devclose(dev);
    close(fd);
    exit(1);
  }
  if (fstat(fd, &st) < 0) {
    fprintf(stderr, "%s : couldn't stat file '%s'\n", argv[0], argv[1]);
    devclose(dev);
    close(fd);
    exit(1);
  }
  size = st.st_size;
  max = size;
  printf("Programming with file '%s' (%d bytes)\n", argv[1], size);
  cur = 0;
  while (size) {
    nb = read(fd, buf, BUFSIZE);
    if (nb < 0) {
      fprintf(stderr, "%s : error reading file '%s'\n", argv[0], argv[1]);
      devclose(dev);
      close(fd);
      exit(1);
    }
    if ((ret = fx2fpgaprog(dev, buf, nb)) < 0) {
      fprintf(stderr, "%s : error %d programming fpga\n", argv[0], ret);
      devclose(dev);
      close(fd);
      exit(1);
    }
    size -= nb;
    cur += nb;
    loadBar(cur, max, 10000, 20);
  }
  if (fx2fpgadone(dev) < 0) {
    fprintf(stderr, "%s : DONE transfer error - configuration failed\n",
	    argv[0]);
    devclose(dev);
    close(fd);
    exit(1);
  } else {
    fprintf(stderr, "%s : DONE completed!\n",
	    argv[0]);
  }
  
  devclose(dev);
  close(fd);
  return 0;
}

void loadBar(int x, int n, int r, int w)
{
  float ratio = x/((float) n);
  printf("%3.3d%% (%d/%d)\r", (int)(ratio*100), x, n);
  fflush(stdout);
}
