/*
 * Copyright (c) 2008 - 2012, Andy Bierman, All Rights Reserved.
 * Copyright (c) 2013 - 2016, Vladimir Vassilev, All Rights Reserved.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.    
 */
/*  FILE: yangcli_tab.c

   NETCONF YANG-based CLI Tool

   See ./README for details

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
18-apr-09    abb      begun; split out from yangcli.c
25-jan-12    abb      Add xpath tab completion provided by Zesi Cai
 
*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <libssh2.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "libtecla.h"
#include "yangcli_wordexp.h"

#include "procdefs.h"
#include "log.h"
#include "ncx.h"
#include "ncxtypes.h"
#include "obj.h"
#include "rpc.h"
#include "status.h"
#include "val.h"
#include "val123.h"
#include "val_util.h"
#include "var.h"
#include "xml_util.h"
#include "yangcli.h"
#include "yangcli_cmd.h"
#include "yangcli_tab.h"
#include "yangcli_util.h"
#include "yangconst.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

/* make sure this is not enabled in the checked in version
 * will mess up STDOUT diplsay with debug messages!!!
 */
/* #define YANGCLI_TAB_DEBUG 1 */
/* #define DEBUG_TRACE 1 */


/********************************************************************
*                                                                   *
*                          T Y P E S                                *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*             F O R W A R D   D E C L A R A T I O N S               *
*                                                                   *
*********************************************************************/


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/


/********************************************************************
 * FUNCTION cpl_add_completion
 *
 *  COPIED FROM /usr/local/include/libtecla.h
 *
 *  Used by libtecla to store one completion possibility
 *  for the current command line in progress
 * 
 *  cpl      WordCompletion *  The argument of the same name that was passed
 *                             to the calling CPL_MATCH_FN() callback function.
 *  line         const char *  The input line, as received by the callback
 *                             function.
 *  word_start          int    The index within line[] of the start of the
 *                             word that is being completed. If an empty
 *                             string is being completed, set this to be
 *                             the same as word_end.
 *  word_end            int    The index within line[] of the character which
 *                             follows the incomplete word, as received by the
 *                             callback function.
 *  suffix       const char *  The appropriately quoted string that could
 *                             be appended to the incomplete token to complete
 *                             it. A copy of this string will be allocated
 *                             internally.
 *  type_suffix  const char *  When listing multiple completions, gl_get_line()
 *                             appends this string to the completion to indicate
 *                             its type to the user. If not pertinent pass "".
 *                             Otherwise pass a literal or static string.
 *  cont_suffix  const char *  If this turns out to be the only completion,
 *                             gl_get_line() will append this string as
 *                             a continuation. For example, the builtin
 *                             file-completion callback registers a directory
 *                             separator here for directory matches, and a
 *                             space otherwise. If the match were a function
 *                             name you might want to append an open
 *                             parenthesis, etc.. If not relevant pass "".
 *                             Otherwise pass a literal or static string.
 * Output:
 *  return              int    0 - OK.
 *                             1 - Error.
 */



/********************************************************************
 * FUNCTION fill_one_parm_completion
 * 
 * fill the command struct for one RPC parameter value
 * check all the parameter values that match, if possible
 *
 * command state is CMD_STATE_FULL or CMD_STATE_GETVAL
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completion state to use
 *    line == line passed to callback
 *    parmval == parm value string to check
 *    word_start == start position within line of the
 *                  word being completed
 *    word_end == word_end passed to callback
 *    parmlen == length of parameter value already entered
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_one_parm_completion (WordCompletion *cpl,
                              completion_state_t *comstate,
                              const char *line,
                              const char *parmval,
                              int word_start,
                              int word_end,
                              int parmlen)


{
    const char  *endchar;
    int          retval;

    /* check no partial match so need to skip this one */
    if (parmlen > 0 &&
        strncmp(parmval,
                &line[word_start],
                parmlen)) {
        /* parameter value start is not the same so skip it */
        return NO_ERR;
    }

    if (comstate->cmdstate == CMD_STATE_FULL) {
        endchar = " ";
    } else {
        endchar = "";
    }

    retval = cpl_add_completion(cpl, line, word_start, word_end,
                           (const char *)&parmval[parmlen],
                           (const char *)"", endchar);

    if (retval != 0) {
        return ERR_NCX_OPERATION_FAILED;
    }

    return NO_ERR;

}  /* fill_one_parm_completion */


/********************************************************************
 * FUNCTION fill_parm_completion
 * 
 * fill the command struct for one RPC parameter value
 * check all the parameter values that match, if possible
 *
 * command state is CMD_STATE_FULL or CMD_STATE_GETVAL
 *
 * INPUTS:
 *    parmobj == RPC input parameter template to use
 *    cpl == word completion struct to fill in
 *    comstate == completion state record to use
 *    line == line passed to callback
 *    word_start == start position within line of the
 *                  word being completed
 *    word_end == word_end passed to callback
 *    parmlen == length of parameter name already entered
 *              this may not be the same as 
 *              word_end - word_start if the cursor was
 *              moved within a long line
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_parm_completion (obj_template_t *parmobj,
                          WordCompletion *cpl,
                          completion_state_t *comstate,
                          const char *line,
                          int word_start,
                          int word_end,
                          int parmlen)
{
    const char            *defaultstr;
    typ_enum_t            *typenum;
    typ_def_t             *typdef, *basetypdef;
    ncx_btype_t            btyp;
    status_t               res;

#ifdef YANGCLI_TAB_DEBUG
    log_debug2("\n*** fill parm %s ***\n", obj_get_name(parmobj));
#endif

    typdef = obj_get_typdef(parmobj);
    if (typdef == NULL) {
        return NO_ERR;
    }

    res = NO_ERR;
    basetypdef = typ_get_base_typdef(typdef);
    btyp = obj_get_basetype(parmobj);

    switch (btyp) {
    case NCX_BT_IDREF:
        {
            const typ_idref_t *idref;
            ncx_module_t *mod;
            ncx_identity_t* identity;
            idref = typ_get_cidref(typdef);

            for (mod = ncx_get_first_session_module();
                 mod != NULL;
                 mod = ncx_get_next_session_module(mod)) {
                 for(identity=(ncx_identity_t*)dlq_firstEntry(&mod->identityQ);
                     identity!=NULL;
                     identity=(ncx_identity_t*)dlq_nextEntry(identity)) {
                     if(ncx123_identity_is_derived_from(identity, idref->base)) {
                         res = fill_one_parm_completion(cpl, comstate, line,
                                                        (const char *)identity->name,
                                                        word_start, word_end, parmlen);
                     }
                 }
            }
        }
	return res;
    case NCX_BT_ENUM:
    case NCX_BT_BITS:
        for (typenum = typ_first_enumdef(basetypdef);
             typenum != NULL && res == NO_ERR;
             typenum = typ_next_enumdef(typenum)) {

#ifdef YANGCLI_TAB_DEBUG
            log_debug2("\n*** found enubit  %s ***\n", typenum->name);
#endif

            res = fill_one_parm_completion(cpl, comstate, line,
                                           (const char *)typenum->name,
                                           word_start, word_end, parmlen);
        }
        return res;
    case NCX_BT_BOOLEAN:
        res = fill_one_parm_completion(cpl, comstate, line,
                                       (const char *)NCX_EL_TRUE,
                                       word_start, word_end, parmlen);
        if (res == NO_ERR) {
            res = fill_one_parm_completion(cpl, comstate, line,
                                           (const char *)NCX_EL_FALSE,
                                           word_start, word_end, parmlen);
        }
        return res;
    case NCX_BT_INSTANCE_ID:
        break;
    default:
        break;
    }

    /* no current values to show;
     * just use the default if any
     */
    defaultstr = (const char *)obj_get_default(parmobj);
    if (defaultstr) {
        res = fill_one_parm_completion(cpl, comstate, line, defaultstr,
                                       word_start, word_end, parmlen);
    }

    return NO_ERR;

}  /* fill_parm_completion */

static status_t
    fill_xpath_predicate_completion (obj_template_t *rpc, obj_template_t *parentObj,
                                    WordCompletion *cpl,
                                    const char *line,
                                    int word_start,
                                    int word_end,
                                    int cmdlen);


/********************************************************************
 * FUNCTION xpath_path_completion_common
 *
 * check and if obj is valid completion and register it with cpl_add_completion
 *
 * INPUTS:
 *    obj == object template to check
 *    cpl == word completion struct to fill in
 *    line == line passed to callback
 *    word_start == start position within line of the
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == command length
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/

static int xpath_path_completion_common(obj_template_t *rpc, obj_template_t * obj,
                                    WordCompletion *cpl,
                                    const char *line,
                                    int word_start,
                                    int word_end,
                                    int cmdlen)
{
    int retval = 0;
    xmlChar         *pathname;
    do {
        /* check if there is a partial command name */
        if (cmdlen > 0 &&
            strncmp((const char *)obj_get_name(obj),
                    &line[word_start],
                    cmdlen)) {
            /* command start is not the same so skip it */
            continue;
        }

        if( !obj_is_data_db(obj)) {
            /* object is either rpc or notification*/
            continue;
        }

        if(!obj_get_config_flag(obj)) {
            const xmlChar* rpc_name;
            rpc_name = obj_get_name(rpc);
            if(0==strcmp((const char*)rpc_name, "create")) continue;
            if(0==strcmp((const char*)rpc_name, "replace")) continue;
            if(0==strcmp((const char*)rpc_name, "delete")) continue;
        }

        if(obj->objtype==OBJ_TYP_CONTAINER || obj->objtype==OBJ_TYP_LIST) {
            pathname = malloc(strlen(obj_get_name(obj))+2);
            assert(pathname);
            strcpy(pathname, obj_get_name(obj));
            pathname[strlen(obj_get_name(obj))] ='/';
            pathname[strlen(obj_get_name(obj))+1] =0;
        } else {
            pathname = malloc(strlen(obj_get_name(obj))+1);
            assert(pathname);
            strcpy(pathname, obj_get_name(obj));
            pathname[strlen(obj_get_name(obj))] =0;
        }
        retval = cpl_add_completion(cpl, line, word_start, word_end,
                                    (const char *)&pathname[cmdlen], "", "");
        free(pathname);
    } while(0);

    return retval;
}

/********************************************************************
 * FUNCTION fill_xpath_children_completion
 * 
 * fill the command struct for one XPath child node
 *
 * INPUTS:
 *    parentObj == object template of parent to check
 *    cpl == word completion struct to fill in
 *    line == line passed to callback
 *    word_start == start position within line of the
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == command length
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_xpath_children_completion (obj_template_t *rpc, obj_template_t *parentObj,
                                    WordCompletion *cpl,
                                    const char *line,
                                    int word_start,
                                    int word_end,
                                    int cmdlen)
{
    int retval;
    int word_iter = word_start + 1;
    // line[word_start] == '/'
    word_start ++;
    cmdlen --;
    while(word_iter <= word_end) {
        if ((line[word_iter] == '/') || (line[word_iter] == '[')) {
            /* The second '/' or predicate condition starting with '[' is found
             * find the top level obj and fill its child completion
             */
            char childName[128];
            int child_name_len = word_iter - word_start;
            strncpy (childName, &line[word_start], child_name_len);
            childName[child_name_len] = '\0';
            if (parentObj == NULL)
                return NO_ERR;
            obj_template_t *childObj = 
                obj_find_child(parentObj,
                               NULL/*obj_get_mod_name(parentObj)*/,
                               (const xmlChar *)childName);
            if(childObj==NULL) {
            	/* no completion possible - the text before / or [ does not resolve to valid obj */
            	return NO_ERR;
            }

            cmdlen = word_end - word_iter;

            /* put the children path with topObj into the recursive
             * lookup function
             */
            if(line[word_iter] == '/') {
                return fill_xpath_children_completion(rpc, childObj,
                                                      cpl,line, word_iter,
                                                      word_end, cmdlen);
            } else if(line[word_iter] == '[') {
                return fill_xpath_predicate_completion(rpc, childObj,
                                                      cpl,line, word_iter,
                                                      word_end, cmdlen);
            }
        }
        word_iter ++;
    }
    obj_template_t * childObj = obj_first_child_deep(parentObj);
    for(;childObj!=NULL; childObj = obj_next_child_deep(childObj)) {

        retval = xpath_path_completion_common(rpc, childObj, cpl, line, word_start, word_end, cmdlen);
        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }
    }
    return NO_ERR;
}  /* fill_xpath_children_completion */

/********************************************************************
 * FUNCTION fill_xpath_predicate_completion
 *
 * fill the command struct for one XPath predicate
 *
 * INPUTS:
 *    parentObj == object template of parent to check
 *    cpl == word completion struct to fill in
 *    line == line passed to callback
 *    word_start == start position within line of the
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == command length
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_xpath_predicate_completion (obj_template_t *rpc, obj_template_t *parentObj,
                                    WordCompletion *cpl,
                                    const char *line,
                                    int word_start,
                                    int word_end,
                                    int cmdlen)
{
    const xmlChar         *pathname;
    int                    retval;
    int word_iter = word_start + 1;
    // line[word_start] == '/'
    word_start ++;
    cmdlen --;
    while(word_iter <= word_end) {
        if (line[word_iter] == ']') {
            // The second ']'
            // fill parentObj child completion
            int predicate_len = word_iter - word_start;
            if (parentObj == NULL)
                return NO_ERR;

            if(((word_iter+1)>word_end) || (line[word_iter+1] != '/')) {
                return NO_ERR;
            }
            word_iter ++;

            cmdlen = word_end - word_iter;

            return fill_xpath_children_completion(rpc, parentObj,
                                                  cpl,line, word_iter,
                                                      word_end, cmdlen);
        }
        word_iter ++;
    }
    obj_template_t * childObj = obj_first_child_deep(parentObj);
    for(;childObj!=NULL; childObj = obj_next_child_deep(childObj)) {
        pathname = obj_get_name(childObj);
        /* check if there is a partial command name */
        if (cmdlen > 0 &&
            strncmp((const char *)pathname,
                    &line[word_start],
                    cmdlen)) {
            /* command start is not the same so skip it */
            continue;
        }

        if( !obj_is_data_db(childObj)) {
            /* object is either rpc or notification*/
            continue;
        }

        if(!obj_get_config_flag(childObj)) {
            const xmlChar* rpc_name;
            rpc_name = obj_get_name(rpc);
            if(0==strcmp((const char*)rpc_name, "create")) continue;
            if(0==strcmp((const char*)rpc_name, "replace")) continue;
            if(0==strcmp((const char*)rpc_name, "delete")) continue;
        }

        if(!obj_is_leaf(childObj)) {
            const xmlChar* rpc_name;
            continue;
        }

        retval = cpl_add_completion(cpl, line, word_start, word_end,
                                    (const char *)&pathname[cmdlen], "", "");
        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }
    }
    return NO_ERR;

} /* fill_xpath_predicate_completion */

/********************************************************************
 * FUNCTION check_find_xpath_top_obj
 * 
 * Check a module for a specified object (full or partial name)
 *
 * INPUTS:
 *    mod == module to check
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == command length
 *
 * RETURNS:
 *   pointer to object if found, NULL if not found
 *********************************************************************/
static obj_template_t * 
    check_find_xpath_top_obj (ncx_module_t *mod,
                              const char *line,
                              int word_start,
                              int cmdlen)
{
    obj_template_t *modObj;
    assert(cmdlen>0);
    modObj = ncx_get_first_object(mod);
    for(; modObj!=NULL; 
        modObj = ncx_get_next_object(mod, modObj)) {
        const xmlChar *pathname = obj_get_name(modObj);
        /* check if there is a partial command name */
        if (strlen(pathname)==cmdlen && !strncmp((const char *)pathname,
                                   &line[word_start],
                                   cmdlen)) {
            return modObj;
        }
    }
    return NULL;

}  /* check_find_xpath_top_obj */



/********************************************************************
 * FUNCTION find_xpath_top_obj
 * 
 * Check all modules for top-level data nodes to save
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completion state in progress
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == command length
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static obj_template_t *
    find_xpath_top_obj (completion_state_t *comstate,
                        const char *line,
                        int word_start,
                        int word_end)
{
    int cmdlen = word_end - word_start;
    obj_template_t *modObj;

    if(cmdlen==0) {
        return NULL;
    }

    if (use_servercb(comstate->server_cb)) {
        modptr_t *modptr;
        for (modptr = (modptr_t *)
                 dlq_firstEntry(&comstate->server_cb->modptrQ);
             modptr != NULL;
             modptr = (modptr_t *)dlq_nextEntry(modptr)) {

            modObj = check_find_xpath_top_obj(modptr->mod, line,
                                              word_start, cmdlen);
            if (modObj != NULL) {
                return modObj;
            }
        }
    } else {
        ncx_module_t * mod = ncx_get_first_session_module();
        for(;mod!=NULL; mod = ncx_get_next_session_module(mod)) {

            modObj = check_find_xpath_top_obj(mod, line,
                                              word_start, cmdlen);
            if (modObj != NULL) {
                return modObj;
            }
        }
    }
    return NULL;
} /* find_xpath_top_obj */


/********************************************************************
 * FUNCTION check_save_xpath_completion
 * 
 * Check a module for saving top-level data objects
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    mod == module to check
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == command length
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    check_save_xpath_completion (
        obj_template_t *rpc,
        WordCompletion *cpl,
        ncx_module_t *mod,
        const char *line,
        int word_start,
        int word_end,
        int cmdlen)
{
    int retval;
    obj_template_t * modObj = ncx_get_first_object(mod);
    for (; modObj != NULL;
         modObj = ncx_get_next_object(mod, modObj)) {

        retval = xpath_path_completion_common(rpc, modObj, cpl, line, word_start, word_end, cmdlen);
        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }
    }
    return NO_ERR;

}  /* check_save_xpath_completion */


/********************************************************************
 * FUNCTION fill_xpath_root_completion
 * 
 * Check all modules for top-level data nodes to save
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completion state in progress
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == command length
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_xpath_root_completion (
        obj_template_t *rpc,
        WordCompletion *cpl,
        completion_state_t *comstate,
        const char *line,
        int word_start,
        int word_end,
        int cmdlen)
{
    status_t res;

    if (use_servercb(comstate->server_cb)) {
        modptr_t *modptr = (modptr_t *)
            dlq_firstEntry(&comstate->server_cb->modptrQ);
        for (; modptr != NULL; modptr = (modptr_t *)dlq_nextEntry(modptr)) {
            res = check_save_xpath_completion(rpc, cpl, modptr->mod, line,
                                              word_start, word_end, cmdlen);
            if (res != NO_ERR) {
                return res;
            }
        }
    } else {
        ncx_module_t * mod = ncx_get_first_session_module();
        for (;mod!=NULL; mod = ncx_get_next_session_module(mod)) {
            res = check_save_xpath_completion(rpc, cpl, mod, line,
                                              word_start, word_end, cmdlen);
            if (res != NO_ERR) {
                return res;
            }
        }
    }
    return NO_ERR;

} /* fill_xpath_root_completion */


/********************************************************************
 * TODO:
 * FUNCTION fill_one_xpath_completion
 *
 * fill the command struct for one RPC operation
 * check all the xpath that match
 *
 * command state is CMD_STATE_FULL or CMD_STATE_GETVAL
 *
 * INPUTS:
 *    rpc == rpc operation to use
 *    cpl == word completion struct to fill in
 *    comstate == completion state in progress
 *    line == line passed to callback
 *    word_start == start position within line of the
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == length of parameter name already entered
 *              this may not be the same as
 *              word_end - word_start if the cursor was
 *              moved within a long line
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_one_xpath_completion (obj_template_t *rpc,
                               WordCompletion *cpl,
                               completion_state_t *comstate,
                               const char *line,
                               int word_start,
                               int word_end,
                               int cmdlen)
{
    (void)rpc;
    int word_iter = word_start + 1;
    // line[word_start] == '/'

    if(word_start==word_end) {
        // this is the case where the cursor is under the / (not after) ignore completion
        return NO_ERR;
    }

    word_start ++;
    cmdlen --;
    while(word_iter <= word_end) {
//        log_write("%c", line[word_iter]);
        if (line[word_iter] == '/') {
              // The second '/' is found
              // TODO: find the top level obj and fill its child completion
//            log_write("more than 1\n");
             obj_template_t * top_obj = find_xpath_top_obj(comstate, line,
                                                          word_start,
                                                          word_iter);

             if(top_obj==NULL) {
                 return NO_ERR;
             }
             cmdlen = word_end - word_iter;

             // put the children path with topObj into the recursive
             // lookup function
             return fill_xpath_children_completion (rpc, top_obj, cpl, line,
                                                    word_iter, word_end,
                                                    cmdlen);
        }
        word_iter ++;
    }

    // The second '/' is not found
    return fill_xpath_root_completion(rpc, cpl, comstate, line, 
                                      word_start, word_end, cmdlen);

}  /* fill_one_xpath_completion */


/********************************************************************
 * FUNCTION fill_one_rpc_completion_parms
 * 
 * fill the command struct for one RPC operation
 * check all the parameters that match
 *
 * command state is CMD_STATE_FULL or CMD_STATE_GETVAL
 *
 * INPUTS:
 *    rpc == rpc operation to use
 *    cpl == word completion struct to fill in
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == length of parameter name already entered
 *              this may not be the same as 
 *              word_end - word_start if the cursor was
 *              moved within a long line
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_one_rpc_completion_parms (obj_template_t *rpc,
                                   WordCompletion *cpl,
                                   completion_state_t *comstate,
                                   const char *line,
                                   int word_start,
                                   int word_end,
                                   int cmdlen)
{
    obj_template_t        *inputobj, *obj;
    const xmlChar         *parmname;
    int                    retval;
    ncx_btype_t            btyp;
    boolean                hasval;

    if(line[word_start] == '/') {
        return fill_one_xpath_completion(rpc, cpl, comstate, line, word_start,
                                         word_end, cmdlen);
    }

    inputobj = obj_find_child(rpc, NULL, YANG_K_INPUT);
    if (inputobj == NULL || !obj_has_children(inputobj)) {
        /* no input parameters */
        return NO_ERR;
    }

    for (obj = obj_first_child_deep(inputobj);
         obj != NULL;
         obj = obj_next_child_deep(obj)) {

        parmname = obj_get_name(obj);

        /* check if there is a partial command name */
        if (cmdlen > 0 &&
            strncmp((const char *)parmname, &line[word_start], cmdlen)) {
            /* command start is not the same so skip it */
            continue;
        }

        btyp = obj_get_basetype(obj);
        hasval = (btyp == NCX_BT_EMPTY) ? FALSE : TRUE;

        retval = cpl_add_completion(cpl, line, word_start, word_end,
                                    (const char *)&parmname[cmdlen],
                                    "", (hasval) ? "=" : " ");

        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }
    }

    return NO_ERR;

}  /* fill_one_rpc_completion_parms */


/********************************************************************
 * FUNCTION fill_one_module_completion_commands
 * 
 * fill the command struct for one command string
 * for one module; check all the commands in the module
 *
 * command state is CMD_STATE_FULL
 *
 * INPUTS:
 *    mod == module to use
 *    cpl == word completion struct to fill in
 *    comstate == completion state in progress
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == length of command already entered
 *              this may not be the same as 
 *              word_end - word_start if the cursor was
 *              moved within a long line
 *
 * OUTPUTS:
 *   cpl filled in if any matching parameters found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_one_module_completion_commands (ncx_module_t *mod,
                                         WordCompletion *cpl,
                                         completion_state_t *comstate,
                                         const char *line,
                                         int word_start,
                                         int word_end,
                                         int cmdlen)
{
    obj_template_t        *obj;
    const xmlChar         *cmdname;
    ncx_module_t          *yangcli_mod;
    int                    retval;
    boolean                toponly;

    yangcli_mod = get_yangcli_mod();
    toponly = FALSE;
    if (mod == yangcli_mod &&
        !use_servercb(comstate->server_cb)) {
        toponly = TRUE;
    }

    /* check all the OBJ_TYP_RPC objects in the module */
    for (obj = ncx_get_first_object(mod);
         obj != NULL;
         obj = ncx_get_next_object(mod, obj)) {

        if (!obj_is_rpc(obj)) {
            continue;
        }

        cmdname = obj_get_name(obj);

        if (toponly && !is_top_command(cmdname)) {
            continue;
        }

        /* check if there is a partial command name */
        if (cmdlen > 0 &&
            strncmp((const char *)cmdname, &line[word_start], cmdlen)) {
            /* command start is not the same so skip it */
            continue;
        }

        retval = cpl_add_completion(cpl, line, word_start, word_end,
                               (const char *)&cmdname[cmdlen],
                               (const char *)"", (const char *)" ");
        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }
    }

    return NO_ERR;

}  /* fill_one_module_completion_commands */


/********************************************************************
 * FUNCTION fill_one_completion_commands
 * 
 * fill the command struct for one command string
 *
 * command state is CMD_STATE_FULL
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completeion state struct in progress
 *    line == line passed to callback
 *    word_start == start position within line of the 
 *                  word being completed
 *    word_end == word_end passed to callback
 *    cmdlen == length of command already entered
 *              this may not be the same as 
 *              word_end - word_start if the cursor was
 *              moved within a long line
 *
 * OUTPUTS:
 *   cpl filled in if any matching commands found
 *
 * RETURNS:
 *   status
 *********************************************************************/
static status_t
    fill_one_completion_commands (WordCompletion *cpl,
                                  completion_state_t *comstate,
                                  const char *line,
                                  int word_start,
                                  int word_end,
                                  int cmdlen)
{
    modptr_t     *modptr;
    status_t      res;

    res = NO_ERR;

    /* figure out which modules to use */
    if (comstate->cmdmodule) {
        /* if foo:\t entered as the current token, then
         * the comstate->cmdmodule pointer will be set
         * to limit the definitions to that module 
         */
        res = fill_one_module_completion_commands
            (comstate->cmdmodule, cpl, comstate, line, word_start,
             word_end, cmdlen);
    } else {
        if (use_servercb(comstate->server_cb)) {
            /* list server commands first */
            for (modptr = (modptr_t *)
                     dlq_firstEntry(&comstate->server_cb->modptrQ);
                 modptr != NULL && res == NO_ERR;
                 modptr = (modptr_t *)dlq_nextEntry(modptr)) {

#ifdef DEBUG_TRACE
                log_debug("\nFilling from server_cb module %s", 
                          modptr->mod->name);
#endif

                res = fill_one_module_completion_commands
                    (modptr->mod, cpl, comstate, line, word_start,
                     word_end, cmdlen);
            }
        }

#ifdef DEBUG_TRACE
        /* use the yangcli top commands every time */
        log_debug("\nFilling from yangcli module");
#endif

        res = fill_one_module_completion_commands
            (get_yangcli_mod(), cpl, comstate, line, word_start,
             word_end, cmdlen);
    }

    return res;

}  /* fill_one_completion_commands */


/********************************************************************
 * FUNCTION is_parm_start
 * 
 * check if current backwards sequence is the start of
 * a parameter or not
 *
 * command state is CMD_STATE_FULL or CMD_STATE_GETVAL
 *
 * INPUTS:
 *    line == current line in progress
 *    word_start == lower bound of string to check
 *    word_cur == current internal parser position 
 *
 * RETURN:
 *   TRUE if start of parameter found
 *   FALSE if not found
 *
 *********************************************************************/
static boolean
    is_parm_start (const char *line,
                   int word_start,
                   int word_cur)
{
    const char            *str;

    if (word_start >= word_cur) {
        return FALSE;
    }

    str = &line[word_cur];
    if (*str == '-') {
        if ((word_cur - 2) >= word_start) {
            /* check for previous char before '-' */
            if (line[word_cur - 2] == '-') {
                if ((word_cur - 3) >= word_start) {
                    /* check for previous char before '--' */
          if (isspace((int)line[word_cur - 3])) {
                        /* this a real '--' command start */
                        return TRUE;
                    } else {
                        /* dash dash part of some value string */
                        return FALSE;
                    }
                } else {
                    /* start of line is '--' */
                    return TRUE;
                }
            } else if (isspace((int)line[word_cur - 2])) {
                /* this is a real '-' command start */
                return TRUE;
            } else {
                return FALSE;
            }
        } else {
            /* start of line is '-' */
            return TRUE;
        }
    } else {
        /* previous char is not '-' */
        return FALSE;
    }

} /* is_parm_start */

static obj_template_t* get_unique_param_w_value_compl_match(obj_template_t* inputobj, char* str)
{
    obj_template_t* childobj;
    unsigned int matchcount;
    /* try to find this parameter name */
    childobj = obj_find_child_str(inputobj, NULL,
                                 (const xmlChar *)str,
                                 strlen(str));
    if (childobj == NULL) {
        matchcount = 0;

        /* try to match this parameter name */
        childobj = obj_match_child_str(inputobj, NULL,
                                       (const xmlChar *)str,
                                       strlen(str),
                                       &matchcount);

        if (childobj && matchcount > 1) {
            /* ambiguous command error
             * but just return no completions
             */
            childobj=NULL;
        }
    }

    /* check if the parameter is an empty, which
     * means another parameter is expected, not a value
     */
    if (childobj!=NULL && obj_get_basetype(childobj) == NCX_BT_EMPTY) {
        childobj=NULL;
    }
    return childobj;
}

void parse_cmdline_completion_variable(char* str, int* param_index, int* value_index)
{
    int i;
    *param_index = -1;
    *value_index=-1;

    /* abc=def - *param_index=0   *eq_sign_index=3 */
    /* -abc=def - *param_index=1  *eq_sign_index=4 */
    /* --abc=def - *param_index=2 *eq_sign_index=5 */
    if(str[0]=='-' && str[1]=='-' && str[2]!='-') {
        *param_index=2;
    } else if(str[0]=='-' && str[1]!='-') {
        *param_index=1;
    } else {
        *param_index=0;
    }
    i=*param_index;
    assert(i>=0);
    while(str[i]!=0 && str[i]!='=') {
        i++;
    }
    if(str[i]=='=') {
        *value_index=i+1;
    }
}


/********************************************************************
 * FUNCTION find_parm_start
 * 
 * go backwards and figure out the previous token
 * from word_end to word_start
 *
 * command state is CMD_STATE_FULL or CMD_STATE_GETVAL
 *
 * INPUTS:
 *    inputobj == RPC operation input node
 *                used for parameter context
 *    cpl == word completion struct to fill in
 *    comstate == completion state struct to use
 *    line == current line in progress
 *    word_start == lower bound of string to check
 *    word_end == current cursor pos in the 'line'
 *              may be in the middle if the user 
 *              entered editing keys; libtecla will 
 *              handle the text insertion if needed
 *   expectparm == address of return expect--parmaeter flag
 *   emptyexit == address of return empty exit flag
 *   parmobj == address of return parameter in progress
 *   tokenstart == address of return start of the current token
 *   
 *
 * OUTPUTS:
 *   *expectparm == TRUE if expecting a parameter
 *                  FALSE if expecting a value
 *   *emptyexit == TRUE if bailing due to no matches possible
 *                 FALSE if there is something to process
 *   *parmobj == found parameter that was entered
 *            == NULL if no parameter expected
 *            == NULL if expected parameter not found
 *               (error returned)
 *  *tokenstart == index within line that the current
 *                 token starts; will be equal to word_end
 *                 if not in the middle of an assignment
 *                 sequence
 *
 * RETURN:
 *   status
 *********************************************************************/
static status_t
    find_parm_start (obj_template_t *inputobj,
                     const char *line,
                     int word_start,
                     int word_end,
                     boolean *expectparm,
                     boolean *emptyexit,
                     obj_template_t **parmobj,
                     int *tokenstart)

{
    yangcli_wordexp_t p;
    int i;

    *parmobj = NULL;
    *emptyexit= FALSE;
    *expectparm=TRUE;

    yangcli_wordexp(line, &p, 0);
    //yangcli_wordexp_dump(&p);
    for (i = 0; i < p.we_wordc; i++) {
        if(p.we_word_line_offset[i]+strlen(p.we_wordv[i])==word_end) {
            /*cursor at the end of a parameter[=value] - completion possible*/
            break;
        } else if((p.we_word_line_offset[i]<=word_end) && (p.we_word_line_offset[i]+strlen(p.we_wordv[i])>word_end)) {
            /*cursor in the middle of parameter[=value] - completion not possible*/
            *emptyexit=TRUE;
            return NO_ERR;
        } else if(0==strcmp("--",p.we_wordv[i])) {
            /* no completion currently supported for secondary arguments e.g. create /interfaces/interface -- loopback=...*/
            *emptyexit=TRUE;
            return NO_ERR;
        }
    }
    if(i==p.we_wordc) {
        /*next parameter starts from scratch*/
        *emptyexit=FALSE;
        *tokenstart = word_end;
    } else if(p.we_wordv[i][0]=='/') {
        /* ignore tokens that are probably default argument Xpath */
        *tokenstart = p.we_word_line_offset[i];
        *emptyexit=FALSE;
    } else {
        int param_index;
        int value_index;
        char* param;
        obj_template_t* childobj;

        *tokenstart = p.we_word_line_offset[i];
        parse_cmdline_completion_variable(p.we_wordv[i], &param_index, &value_index);
        if(value_index!=-1) {
            unsigned int param_len = value_index-1;
            param = malloc(param_len+1);
            memcpy(param,p.we_wordv[i],param_len);
            param[param_len]=0; 
            *tokenstart=param_len+*tokenstart;
            childobj=get_unique_param_w_value_compl_match(inputobj,param);
            free(param);
            if(childobj==NULL) {
                *emptyexit = TRUE;
            } else {
                *parmobj = childobj;
                *expectparm=FALSE;
            }
        } else if(param_index!=-1) {
            *tokenstart=param_index+*tokenstart;
            *emptyexit = FALSE;
        }
    }
    yangcli_wordfree(&p);
    return NO_ERR;
}

/********************************************************************
 * FUNCTION parse_backwards_parm
 * 
 * go through the command line backwards
 * from word_end to word_start, figuring
 *
 * command state is CMD_STATE_FULL
 *
 * This function only used when there are allowed
 * to be multiple parm=value pairs on the same line
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completion state struct to use
 *    line == current line in progress
 *    word_start == lower bound of string to check
 *    word_end == current cursor pos in the 'line'
 *              may be in the middle if the user 
 *              entered editing keys; libtecla will 
 *              handle the text insertion if needed
 *
 * OUTPUTS:
 *   parameter or value completions addeed if found
 *
 * RETURN:
 *   status
 *********************************************************************/
static status_t
    parse_backwards_parm (WordCompletion *cpl,
                          completion_state_t *comstate,
                          const char *line,
                          int word_start,
                          int word_end)

{
    obj_template_t        *rpc, *inputobj, *parmobj;
    status_t               res;
    int                    tokenstart;
    boolean                expectparm, emptyexit;

    if (word_end == 0) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    res = NO_ERR;
    parmobj = NULL;
    tokenstart = 0;
    expectparm = FALSE;
    emptyexit = FALSE;
    rpc = comstate->cmdobj;
    inputobj = comstate->cmdinput;

    res = find_parm_start(inputobj, line, word_start, word_end, &expectparm,
                          &emptyexit, &parmobj, &tokenstart);
    if (res != NO_ERR || emptyexit) {
        return res;
    }

    if (expectparm) {
        /* add all the parameters, even those that
         * might already be entered (oh well)
         * this is OK for leaf-lists but not leafs
         */

#ifdef YANGCLI_TAB_DEBUG
        log_debug2("\n*** fill one RPC %s parms ***\n", obj_get_name(rpc));
#endif

        res = fill_one_rpc_completion_parms(rpc, cpl, comstate, line,
                                            tokenstart, word_end, 
                                            word_end - tokenstart);
    } else if (parmobj) {
        /* have a parameter in progress and the
         * token start is supposed to be the value
         * for this parameter
         */

#ifdef YANGCLI_TAB_DEBUG
        log_debug2("\n*** fill one parm in backwards ***\n");
#endif

        res = fill_parm_completion(parmobj, cpl, comstate, line, tokenstart,
                                   word_end, word_end - tokenstart);
    } /* else nothing to do */

    return res;

} /* parse_backwards_parm */


/********************************************************************
 * FUNCTION fill_parm_values
 * 
 * go through the command line 
 * from word_start to word_end, figuring
 * out which value is entered
 *
 * command state is CMD_STATE_GETVAL
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completion state struct to use
 *    line == current line in progress
 *    word_start == lower bound of string to check
 *    word_end == current cursor pos in the 'line'
 *              may be in the middle if the user 
 *              entered editing keys; libtecla will 
 *              handle the text insertion if needed
 *
 * OUTPUTS:
 *   value completions addeed if found
 *
 * RETURN:
 *   status
 *********************************************************************/
static status_t
    fill_parm_values (WordCompletion *cpl,
                      completion_state_t *comstate,
                      const char *line,
                      int word_start,
                      int word_end)
{
    status_t               res;

    /* skip starting whitespace */
    while (word_start < word_end && isspace((int)line[word_start])) {
        word_start++;
    }

    /*** NEED TO CHECK FOR STRING QUOTES ****/

#ifdef YANGCLI_TAB_DEBUG
    log_debug2("\n*** fill parm values ***\n");
#endif

    res = fill_parm_completion(comstate->cmdcurparm, cpl, comstate, line,
                               word_start, word_end, word_end - word_start);

    return res;

} /* fill_parm_values */

/********************************************************************
 * FUNCTION strip_predicate
 *
 * INPUTS:
 *    line == original line containing predicates
 *
 * OUTPUTS:
 *    line_striped == at least equal in size buffer to the original line to contain the result
 *
 * EXAMPLE:
 *    /interfaces/interface[name='ge0']/mtu -> /interfaces/interface/mtu
 *********************************************************************/
static void strip_predicate(char* line, char* line_striped)
{
    unsigned int i,j;
    unsigned int len;
    int predicate_open=0;
    int double_quote_open=0;
    int single_quote_open=0;
    unsigned int predicate_open_j;

    len = strlen(line);

    for(i=0,j=0;i<len;i++) {
        if(predicate_open) {
            if(single_quote_open) {
                if(line[i]=='\'') {
                    single_quote_open=0;
                }
            } else if(double_quote_open) {
                if(line[i]=='"') {
                    double_quote_open=0;
                }
            } else {
                if(line[i]==']') {
                    predicate_open = 0;
                    j = predicate_open_j;
                    continue;
                }
            }
        } else {
            if(single_quote_open) {
                if(line[i]=='\'') {
                    single_quote_open=0;
                }
            } else if(double_quote_open) {
                if(line[i]=='"') {
                    double_quote_open=0;
                }
            } else {
                if(line[i]=='[') {
                    predicate_open=1;
                    predicate_open_j = j;
                }
            }
        }
        line_striped[j++]=line[i];
    }
    line_striped[j]=0;

} /* strip_predicate */


/********************************************************************
 * FUNCTION fill_completion_commands
 * 
 * go through the available commands to see what
 * matches should be returned
 *
 * command state is CMD_STATE_FULL
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    comstate == completion state struct to use
 *    line == current line in progress
 *    word_end == current cursor pos in the 'line'
 *              may be in the middle if the user 
 *              entered editing keys; libtecla will 
 *              handle the text insertion if needed
 * OUTPUTS:
 *   comstate
 * RETURN:
 *   status
 *********************************************************************/
static status_t
    fill_completion_commands (WordCompletion *cpl,
                              completion_state_t *comstate,
                              const char *line,
                              int word_end)

{
    obj_template_t        *inputobj;
    const char            *str, *cmdend, *cmdname, *match;
    xmlChar               *buffer;
    ncx_node_t             dtyp;
    status_t               res;
    int                    word_start, cmdlen;
    uint32                 retlen;
    boolean                done, equaldone;
    char                   *line_striped;

    res = NO_ERR;
    str = line;
    cmdend = NULL;
    word_start = 0;

    /* skip starting whitespace */
    while ((str < &line[word_end]) && isspace((int)*str)) {
        str++;
    }

    /* check if any real characters entered yet */
    if (word_end == 0 || str == &line[word_end]) {
        /* found only spaces so far or
         * nothing entered yet 
         */
        res = fill_one_completion_commands(cpl, comstate, line, word_end,
                                           word_end, 0);
        if (res != NO_ERR) {
            cpl_record_error(cpl, get_error_string(res));
        }
        return res;
    }

    /* else found a non-whitespace char in the buffer that
     * is before the current tab position
     * check what kind of command line is in progress
     */
    if (*str == '@' || *str == '$') {
        /* at-file-assign start sequence OR
         * variable assignment start sequence 
         * may deal with auto-completion for 
         * variables and files later
         * For now, skip until the end of the 
         * assignment is found or word_end hit
         */
        comstate->assignstmt = TRUE;
        equaldone = FALSE;

        while ((str < &line[word_end]) && 
               !isspace((int)*str) && (*str != '=')) {
            str++;
        }

        /* check where the string search stopped */
        if (isspace((int)*str) || *str == '=') {
            /* stopped past the first word
             * so need to skip further
             */
      if (isspace((int)*str)) {
                /* find equals sign or word_end */
                while ((str < &line[word_end]) && 
                       isspace((int)*str)) {
                    str++;
                }
            }

            if ((*str == '=') && (str < &line[word_end])) {
                equaldone = TRUE;
                str++;  /* skip equals sign */

                /* skip more whitespace */
                while ((str < &line[word_end]) && 
                       isspace((int)*str)) {
                    str++;
                }
            }

            /* got past the '$foo =' part 
             * now looking for a command 
             */
            if (str < &line[word_end]) {
                /* go to next part and look for
                 * the end of this start command
                 */
                word_start = (int)(str - line);
            } else if (equaldone) {
                res = fill_one_completion_commands(cpl, comstate, line,
                                                   word_end, word_end, 0);
                if (res != NO_ERR) {
                    cpl_record_error(cpl, get_error_string(res));
                }
                return res;
            } else {
                /* still inside the file or variable name */
                return res;
            }
        } else {
            /* word_end is still inside the first
             * word, which is an assignment of
             * some sort; not going to show
             * any completions for user vars and files yet
             */
            return res;
        }
    } else {
        /* first word starts with a normal char */
        word_start = (int)(str - line);
    }

    /* the word_start var is set to the first char
     * that is supposed to be start command name
     * the 'str' var points to that char
     * check if it is an entire or partial command name
     */
    cmdend = str;
    while (cmdend < &line[word_end] && !isspace((int)*cmdend)) {
        cmdend++;
    }
    cmdlen = (int)(cmdend - str);

    /* check if still inside the start command */
    if (cmdend == &line[word_end]) {
        /* get all the commands that start with the same
         * characters for length == 'cmdlen'
         */
        res = fill_one_completion_commands(cpl, comstate, line, word_start,
                                           word_end, cmdlen);
        if (res != NO_ERR) {
            cpl_record_error(cpl, get_error_string(res));
        }
        return res;
    }

    /* not inside the start command so get the command that
     * was selected if it exists; at this point the command
     * should be known, so find it
     */
    cmdname = &line[word_start];

    buffer = xml_strndup((const xmlChar *)cmdname, (uint32)cmdlen);

    if (buffer == NULL) {
        if (cpl != NULL) {
            cpl_record_error(cpl, get_error_string(ERR_INTERNAL_MEM));
        }
        return 1;
    }
    retlen = 0;
    dtyp = NCX_NT_OBJ;
    res = NO_ERR;
    comstate->cmdobj = (obj_template_t *)
        parse_def(comstate->server_cb, &dtyp, buffer, &retlen, &res);
    m__free(buffer);

    if (comstate->cmdobj == NULL) {
        if (cpl != NULL) {
            cpl_record_error(cpl, get_error_string(res));
        }
        return 1;
    }

    /* have a command that is valid
     * first skip any whitespace after the
     * command name
     */
    str = cmdend;
    while (str < &line[word_end] && isspace((int)*str)) {
        str++;
    }
    /* set new word_start == word_end */
    word_start = (int)(str - line);

    /* check where E-O-WSP search stopped */
    if (str == &line[word_end]) {
        /* stopped before entering any parameters */
        res = fill_one_rpc_completion_parms(comstate->cmdobj, cpl, 
                                            comstate, line,
                                            word_start, word_end, 0);
        return res;
    }


    /* there is more text entered; check if this rpc
     * really has any input parameters or not
     */
    inputobj = obj_find_child(comstate->cmdobj, NULL, YANG_K_INPUT);
    if (inputobj == NULL ||
        !obj_has_children(inputobj)) {
        /* no input parameters expected */
        return NO_ERR;
    } else {
        comstate->cmdinput = inputobj;
    }

    /* got some edited text past the last 
     * quoted string so figure out the
     * parm in progress and check for completions
     */
    res = parse_backwards_parm(cpl, comstate, line, word_start, word_end);
    return res;

} /* fill_completion_commands */


/********************************************************************
 * FUNCTION fill_completion_yesno
 * 
 * go through the available commands to see what
 * matches should be returned
 *
 * command state is CMD_STATE_YESNO
 *
 * INPUTS:
 *    cpl == word completion struct to fill in
 *    line == current line in progress
 *    word_end == current cursor pos in the 'line'
 *              may be in the middle if the user 
 *              entered editing keys; libtecla will 
 *              handle the text insertion if needed
 * OUTPUTS:
 *   comstate
 * RETURN:
 *   status
 *********************************************************************/
static status_t
    fill_completion_yesno (WordCompletion *cpl,
                           const char *line,
                           int word_end)
{
    const char            *str;
    int                    word_start, retval;

    str = line;
    word_start = 0;

    /* skip starting whitespace */
    while ((str < &line[word_end]) && isspace((int)*str)) {
        str++;
    }

    /* check if any real characters entered yet */
    if (word_end == 0 || str == &line[word_end]) {
        /* found only spaces so far or
         * nothing entered yet 
         */

        retval = cpl_add_completion(cpl, line, word_start, word_end,
                                    (const char *)NCX_EL_YES, 
                                    (const char *)"", (const char *)" ");
        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }


        retval = cpl_add_completion(cpl, line, word_start, word_end,
                                    (const char *)NCX_EL_NO,
                                    (const char *)"", (const char *)" ");
        if (retval != 0) {
            return ERR_NCX_OPERATION_FAILED;
        }
    }

    /*** !!!! ELSE NOT GIVING ANY PARTIAL COMPLETIONS YET !!! ***/

    return NO_ERR;

}  /* fill_completion_yesno */


/*.......................................................................
 *
 * FUNCTION yangcli_tab_callback (word_complete_cb)
 *
 *   libtecla tab-completion callback function
 *
 * Matches the CplMatchFn typedef
 *
 * From /usr/lib/include/libtecla.h:
 * 
 * Callback functions declared and prototyped using the following macro
 * are called upon to return an array of possible completion suffixes
 * for the token that precedes a specified location in the given
 * input line. It is up to this function to figure out where the token
 * starts, and to call cpl_add_completion() to register each possible
 * completion before returning.
 *
 * Input:
 *  cpl  WordCompletion *  An opaque pointer to the object that will
 *                         contain the matches. This should be filled
 *                         via zero or more calls to cpl_add_completion().
 *  data           void *  The anonymous 'data' argument that was
 *                         passed to cpl_complete_word() or
 *                         gl_customize_completion()).
 *  line     const char *  The current input line.
 *  word_end        int    The index of the character in line[] which
 *                         follows the end of the token that is being
 *                         completed.
 * Output
 *  return          int    0 - OK.
 *                         1 - Error.
 */
int
    yangcli_tab_callback (WordCompletion *cpl, 
                          void *data,
                          const char *line, 
                          int word_end)
{
    completion_state_t   *comstate;
    status_t              res;
    int                   retval;

#ifdef DEBUG
    if (!cpl || !data || !line) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 1;
    }
#endif

    retval = 0;

    /* check corner case that never has any completions
     * no matter what state; backslash char only allowed
     * for escaped-EOLN or char-sequence that we can't guess
     */
    if (word_end > 0 && line[word_end] == '\\') {
        return retval;
    }

    comstate = (completion_state_t *)data;

    if (comstate->server_cb && comstate->server_cb->climore) {
        comstate->cmdstate = CMD_STATE_MORE;
    }

    switch (comstate->cmdstate) {
    case CMD_STATE_FULL:
        res = fill_completion_commands(cpl, comstate, line, word_end);
        if (res != NO_ERR) {
            retval = 1;
        }
        break;
    case CMD_STATE_GETVAL:
        res = fill_parm_values(cpl, comstate, line, 0, word_end);
        if (res != NO_ERR) {
            retval = 1;
        }
        break;
    case CMD_STATE_YESNO:
        res = fill_completion_yesno(cpl, line, word_end);
        if (res != NO_ERR) {
            retval = 1;
        }
        break;
    case CMD_STATE_MORE:
        /*** NO SUPPORT FOR MORE MODE YET ***/
        break;
    case CMD_STATE_NONE:
        /* command state not initialized */
        SET_ERROR(ERR_INTERNAL_VAL);
        retval = 1;
        break;
    default:
        /* command state garbage value */
        SET_ERROR(ERR_INTERNAL_VAL);
        retval = 1;
    }

    return retval;

} /* yangcli_tab_callback */


/* END yangcli.c */
