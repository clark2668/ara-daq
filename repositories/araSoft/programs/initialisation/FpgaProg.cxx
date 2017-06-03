#include <iostream>
#include <fstream>



//usbInterface includes
#include "usbInterface.h"

#define buffSize 64

static void ctlWriteCBF(struct libusb_transfer *transfer);

using namespace std;


ifstream::pos_type size;
char * data;
unsigned char * temp;
char * fileName;


int main(int argc, char **argv){
  if(argc == 2)
    fileName = argv[1];
  else
    {
      cout << "Correct usage ./FpgaProg <filename.bin>" << endl;
      return 1;
    }

  cout << "Opening file " << fileName << endl;

  ifstream inFile(fileName, ios::in | ios::binary);
  if(inFile.is_open())
    cout <<fileName << " opened" << endl;
  else
    cout << "Unable to open file" << endl;
  inFile.seekg(0, ios::end);
  size = inFile.tellg();

  //  cout << "Size of file is " << size << endl;

  inFile.seekg(0, ios::beg);
  
  data = new char[size];
  temp = new unsigned char[10];
  inFile.read(data,size);

  inFile.close();

  usbInterface *fTheUSB = new usbInterface(0x04b4, 0x9999);

  int retval = fTheUSB->controlTransfer(temp, 0, 0xb1,0,0, ctlWriteCBF);
  if(retval!=1)
    cout << "fpga SU vendor command error, retval " << retval << endl;

  for(int i=0; i<size; i++)
    {
      if(!(i%buffSize))
	{
	  retval = fTheUSB->controlTransfer((unsigned char*)&data[i], 0, 0xb2,0,buffSize, ctlWriteCBF);

	  if(retval!=1)
	    {
	      cout << "Error in fpga transfer stage " << endl;
	      return -1;
	      //	      break;
	    }
	  if(!((i/buffSize)%(size/(buffSize*10))))
	     cout << (i/(buffSize*(size/(buffSize*100)))) << "% complete" << endl;
	}	

    }

  if(size%buffSize)
    {
      //      cout << "last packet length " << (size%buffSize) << endl;

      retval = fTheUSB->controlTransfer((unsigned char*)&data[size-(size%buffSize)], 0, 0xb2,0,size%buffSize, ctlWriteCBF);
      if(retval!=1)
	cout << "Failed to send transfer" << endl;

    }
  retval = fTheUSB->controlTransfer(temp, 1, 0xb3,0,2, ctlWriteCBF);
  if(retval==1)
    cout << "DONE transfer completed, FPGA configuration completed successfully" << endl;
  else 
    cout << "DONE transfer error, FPGA not configured - retval " << retval << endl; 

  delete data;
  delete temp;  
  delete fTheUSB;
}


static void ctlWriteCBF(struct libusb_transfer *transfer)
{

}
