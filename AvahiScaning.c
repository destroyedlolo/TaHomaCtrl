/* Wait for TaHoma's advertising
 */

#include "TaHomaCtl.h"

void func_scan(const char *){
		/* Remove old references */
	clean(&tahoma);
	clean(&token);
	clean(&url);
}
