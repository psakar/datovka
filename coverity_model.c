/* Coverity Scan model
 *
 * This is a modelling file for Coverity Scan. Modelling helps to avoid false
 * positives.
 *
 * - A model file can't import any header files.
 * - Therefore only some built-in primitives like int, char and void are
 *   available but not wchar_t, NULL etc.
 * - Modelling doesn't need full structs and typedefs. Rudimentary structs
 *   and similar types are sufficient.
 * - An uninitialized local pointer is not an error. It signifies that the
 *   variable could be either NULL or have some data.
 *
 * Coverity Scan doesn't pick up modifications automatically. The model file
 * must be uploaded by an admin in the analysis settings of
 * http://scan.coverity.com/projects/3079
 */

#define NULL ((void *)0)

char *sanitised_tz_val(const char *tz_val)
{
	char *ret = NULL;
	if (NULL != tz_val) {
		ret = strdup();
	}
	__coverity_tainted_string_sanitize_content__(ret);
	return ret;
}
