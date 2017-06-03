//
// dbWriteIdentify writes a daughterboard's identification EEPROM page 0.
// 
// The remaining pages are undefined right now: there are several
// possibilities: either calibration data (unlikely given that it's only
// a 32k EEPROM and there are a lot more cal points for a DDA),
// operation data (more likely - as in you store the nominal operating
// Vdly/Vadj for a DDA, and the nominal thresholds for a TDA), and
// maybe tracking data as well.
//
// dbWriteIdentify takes a file containing the write page info, in ASCII.
// Format is one entry per line:
// line 0: page 0 version (max 255)
// line 1: daughterboard name (max 6 characters)
// line 2: daughterboard revision (max 1 character)
// line 3: daughterboard serial number (max 8 characters)
// line 4: daughterboard build date (max 8 characters- MM/DD/YY)
// line 5: daughterboard build location/by (max 8 characters)
// line 6: cal date (same as build date)
// line 7: cal by (same as build by)
// line 8: commission date (same as build date)
// line 9: installation location (max 8 characters)
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>

extern "C" 
{   
 #include "araSoft.h"
 #include "atriComLib/atriCom.h"
}

#include <getopt.h>

// why don't we have this defined anywhere. Hmm.
#define ATRI_NUM_DAUGHTERS 4

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <sys/types.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>


using std::vector;
using std::string;
using std::ifstream;
using std::cerr;
using std::cout;
using std::endl;
using std::hex;
using std::dec;

template <class T>
bool from_string(T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&)) {
  std::istringstream iss(s);
  return !(iss >> f >> t).fail();
}

int main(int argc, char **argv) {  
  char page0[64];
  bool eos = false;
  int i;
  const char *p;
  bool test_mode = false;
  bool read_mode = false;
  int daughter = -1;
  int retval;
  int type = -1;
  // Command line processing:
  // We require a page 0 file name, plus a --daughter (or -D) argument
  // and a --type (or -t) argument. -t can be ASCII ("DDA","TDA","DRSV9",
  // or "DRSV10")
  // If --simulate (or -s) is specified, we don't actually write (or even open
  // the control socket), just dump what we would write.
  // If --read (or -r) is specified (conflicts with --test) we instead read
  // the page 0 file name and write it into the same format text file.
  static struct option options[] = 
    {
      {"simulate", no_argument, 0, 's'},
      {"read", no_argument, 0, 'r'},
      {"daughter", required_argument, 0, 'D'},
      {"type", required_argument, 0, 't'}
    };
  int c;
  while (1) {
    int option_index;
    c = getopt_long(argc, argv, "srD:t:", options, &option_index);
    if (c == -1) break;
    switch (c) {
     case 's': test_mode = true; break;
     case 'r': read_mode = true; break;
     case 'D': daughter = strtoul(optarg, NULL, 0); 
      if (daughter < 1 || daughter > ATRI_NUM_DAUGHTERS) {
	cerr << argv[0] << " --daughter options are 1-4" << endl;
	return 1;
      }
      // make it zero based
      daughter--;
      break;
     case 't': if (!strcmp(optarg, "DDA")) type = 0;
      if (!strcmp(optarg, "TDA")) type = 1;
      if (!strcmp(optarg, "DRSV9")) type = 2;
      if (!strcmp(optarg, "DRSV10")) type = 3;
      if (type < 0) {
	char *rp;
	type = strtoul(optarg, &rp, 0);
	if (rp == optarg || (type > 3)) {
	  cerr << argv[0] << " --type options are DDA, TDA, DRSV9, DRSV10, or 0-3" << endl;
	  return 1;
	}
      }
      break;
     default: exit(1);
    }     
  }
  if (test_mode && read_mode) {
    cerr << argv[0] << " : --read and --simulate are exclusive" << endl;
    return 1;
  }
  if (optind == argc) {
    cerr << argv[0] << ": need a page 0 info file name" << endl;
    return 1;
  }
  if (daughter < 0 || type < 0) {
    cerr << argv[0] << ": need --daughter (-D) and --type (-t) options"
         << endl;
    return 1;
  }
  vector<string> text_file;
  string temp;
  if (!read_mode) {
    ifstream ifs(argv[optind]);
    if (!ifs.is_open()) {
      cerr << argv[0] << ": " << argv[optind] << " is not readable" << endl;
      return 1;
    }
    while (getline(ifs, temp))
      text_file.push_back(temp);
    
    unsigned int page0_version;
    if (!from_string<unsigned int>(page0_version, text_file[0], std::dec)) {
      cerr << argv[0] << ": line 0 must be a decimal number from 0-255" << endl;
      exit(1);
    }
    if (page0_version > 255) {
      cerr << argv[0] << ": line 0 must be a decimal number from 0-255" << endl;
      exit(1);
    }
    
    cout << "Page 0 Version: " << page0_version << endl;
    cout << "Daughterboard Name: " << text_file[1] << endl;
    cout << "Revision: " << text_file[2] << endl;
    cout << "Serial Number: " << text_file[3] << endl;
    cout << "Build Date: " << text_file[4] << endl;
    cout << "Build By: " << text_file[5] << endl;
    cout << "Cal Date: " << text_file[6] << endl;
    cout << "Cal By: " << text_file[7] << endl;
    cout << "Commission Date: " << text_file[8] << endl;
    cout << "Installation Location: " << text_file[9] << endl;
    
    cout << endl;
    cout << "Raw Page 0: " << endl;
    
    // 0, 1, 2 are chars 0->7
    page0[0] = (unsigned char) page0_version;
    eos = false;
    p = text_file[1].c_str();
    for (i=0;i<6;i++) {
      if (!eos)
	page0[1+i] = p[i];
      else
	page0[1+i] = 0;
      
      if (p[i] == 0x00)
	eos = true;
    }
    eos = false;
    p = text_file[2].c_str();
    page0[7] = p[0];
    
    // 3 is chars 8-15
    p = text_file[3].c_str();
    eos = false;
    for (i=0;i<8;i++) {
      if (!eos)
	page0[8+i] = p[i];
      else
	page0[8+i] = 0x00;
      
      if (p[i] == 0x00)
	eos = true;
    }
    
    // 4 is 16-23
    p = text_file[4].c_str();
    eos = false;
    for (i=0;i<8;i++) {
      if (!eos)
	page0[16+i] = p[i];
      else
	page0[16+i] = 0x00;
      
      if (p[i] == 0x00)
	eos = true;
    }
    
    // 5 is 24-31
    p = text_file[5].c_str();
    eos = false;
    for (i=0;i<8;i++) {
      if (!eos)
	page0[24+i] = p[i];
      else
	page0[24+i] = 0x00;
      
      
      if (p[i] == 0x00)
	eos = true;
    }
    
    // 32-39
    p = text_file[6].c_str();
    eos = false;
    for (i=0;i<8;i++) {
      if (!eos)
	page0[32+i] = p[i];
      else
	page0[32+i] = 0x00;
      
      
      if (p[i] == 0x00)
	eos = true;
    }
    
    // 40-47
    p = text_file[7].c_str();
    eos = false;
    for (i=0;i<8;i++) {
      if (!eos)
	page0[40+i] = p[i];
      else
	page0[40+i] = 0x00;
      
      
      if (p[i] == 0x00)
	eos = true;
    }
    
    // 48-55
    p = text_file[8].c_str();
    eos = false;
    for (i=0;i<8;i++) {
      if (!eos)
	page0[48+i] = p[i];
      else
	page0[48+i] = 0x00;
      
      
      if (p[i] == 0x00)
	eos = true;
    }
    
    // 56-63
    p = text_file[9].c_str();
    eos = false;
    for (i=0;i<8;i++) {
      if (!eos)
	page0[56+i] = p[i];
      else
	page0[56+i] = 0x00;
      if (p[i] == 0x00)
	eos = true;
    }
    
    for (i=0;i<64;i++) {
      cout << "0x" << hex << (((unsigned int) (page0[i]))&0xFF);
      if (!((i+1)%8))
	cout << endl;
      else 
	cout << " ";
    }
    if (!test_mode) {
      cout << "Attempting to write page0 of daughter type " << atriDaughterTypeStrings[type]
	   << " on stack " << atriDaughterStackStrings[daughter] 
	   << " at address " << std::hex << (unsigned int) atriEepromAddressMap[type] << endl;
      // We now have 64 bytes to write to the EEPROM.
      int auxFd = openConnectionToAtriControlSocket();
      if (auxFd < 0) {
	return 1;
      }
      if (atriEepromAddressMap[type] == 0x00) {
	cerr << " -- I have no idea what the EEPROM address is on a "
	     << atriDaughterTypeStrings[type]
	     << endl;
	close(auxFd);
	return 1;
      }
       // we can only write 16 bytes at a time
       // wow do I need to rewrite the I2C controller.
       // so we actually only write 8 bytes at a time to keep things simple
      for (int i=0;i<16;i++) 
	 {
	    uint8_t buf[10];
	    buf[0] = 0x00;
	    buf[1] = i*4;
	    memcpy(&(buf[2]), page0+i*4, 4);
	    if ((retval = writeToAtriI2C(auxFd, (AtriDaughterStack_t) daughter, atriEepromAddressMap[type], 6,
					 buf))) {
	       cerr << " -- Error " << retval << " writing to page 0 ("
		 << i*4 << "-" << i*4+3 << ")" << endl;
	       close(auxFd);
	       return 1;
	    }	    
	    usleep(500);
	 }       
       cout << " -- Write successful." << endl;
       closeConnectionToAtriControlSocket(auxFd);
    }     
  }
  cout <<endl;

  return 0;
}
