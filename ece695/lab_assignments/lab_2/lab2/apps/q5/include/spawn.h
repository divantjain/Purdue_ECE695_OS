#ifndef __USERPROG__
#define __USERPROG__

typedef struct missile_code {
  int numprocs;
  char really_important_char;
} missile_code;

typedef struct molecule_count {
  int n;
  int o;
  int n2;
  int o2;
  int no2;
  int o3;
} molecule_count;

typedef struct lock_struct {
  int num_nitrogen;
  int num_oxygen;
  int num_n2;
  int num_o2;
  int num_no2;
  int num_o3;
  lock_t n_lock;
  lock_t o_lock;
  lock_t n2_lock;
  lock_t o2_lock;
  cond_t two_n_available;
  cond_t two_o_available;
  cond_t one_n2_available;
  cond_t two_o2_available;
  cond_t three_o2_available;
} lock_struct;

#define FILENAME_TO_RUN "krypton.dlx.obj"
#define OXYGEN_INJECT_FILENAME_TO_RUN "oxygen_inject.dlx.obj"
#define NITROGEN_INJECT_FILENAME_TO_RUN "nitrogen_inject.dlx.obj"
#define OXYGEN_FILENAME_TO_RUN "oxygen.dlx.obj"
#define NITROGEN_FILENAME_TO_RUN "nitrogen.dlx.obj"
#define OZONE_FILENAME_TO_RUN "ozone.dlx.obj"
#define NO2_FILENAME_TO_RUN "no2.dlx.obj"

#define TRUE 1
#define FALSE 0

#endif
