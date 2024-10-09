#ifndef NEXTION_DECLARATIONS_H
#define NEXTION_DECLARATIONS_H

#include <Nextion.h>

// Deklarasi objek-objek Nextion
extern NexButton bpagelogin;
extern NexButton bpagehome;
extern NexButton bsubmit;
extern NexButton balarmsetting;
extern NexButton blogin;
extern NexButton btutup;
extern NexButton bsubmit1;
extern NexButton bsimpan;
extern NexButton bback;

extern NexDSButton bBar;
extern NexDSButton bKPa;
extern NexDSButton bPSi;

extern NexPage home;
extern NexPage settingPage;
extern NexPage loginpage;
extern NexPage alarmpage;
extern NexPage settingsn;
extern NexPage setupPassword;


// Deklarasi fungsi-fungsi callback
void bsavePopCallback(void *ptr);
void bPageloginPopCallback(void *ptr);
void bSubmitPopCallback(void *ptr);
void bpPasswordPopCallback(void *ptr);
void badminpasswordPopCallback(void *ptr);
void balarmsettingPopCallback(void *ptr);
void bbackPopCallback(void *ptr);
void bPagehomePopCallback(void *ptr);
void bBarPopCallback(void *ptr);
void bKPaPopCallback(void *ptr);
void bPSiPopCallback(void *ptr);

// Deklarasi fungsi setup Nextion
void setupNextiontombol();

#endif // NEXTION_DECLARATIONS_H