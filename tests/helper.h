

#ifndef _HELPER_H_
#define _HELPER_H_


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @brief Reads content of file into memory.
 */
char * internal_read_file(const char *fname, size_t *length, const char *mode);


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* _HELPER_H_ */
