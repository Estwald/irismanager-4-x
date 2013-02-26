// Defines for PS3 3.55
#define strncmp                     0x4e6d8                                                                                                                                    
#define strcpy                      0x4e684                                                                                                                                            
#define strlen                      0x4e6ac                                                                                                                                            
#define alloc                       0x60b78                                                                                                                                            
#define free                        0x60fb4

#define memory_patch_func           0x2b329c
#define pathdup_from_user           0x18dc68                                                                                                                                           
#define open_mapping_table_ext      0x7fff00                                                                                                                                           

/* Common Symbols PL3 */

#define memcpy                      0x7c3a4
#define memset                      0x4e4d8
#define copy_from_user              0x0f8c0
#define copy_to_user                0x0f6a4

#define alloc_and_copy_from_user    0x18DE28
#define strdup_from_user            0x192B8C
#define strdup                      0x192B8C

#define perm_patch_func             0x0e7f0
#define rtoc_entry_1		        0x1030
#define rtoc_entry_2		       -0x5b80

