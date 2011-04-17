#include "hashtest.h"
#include "memlink_client.h"

int main()
{
#ifdef DEBUG
	logfile_create("test.log", 3);
	//logfile_create("stdout", 3);
#endif
	HashTable* ht;
	char key[64];
	int  valuesize = 8;
	char val[64];
	unsigned int maskformat[3]   = {4, 3, 1};	
	unsigned int maskarray[4][3] = {{ 7, 2, 1}, {6, 2, 1}, { 4, 1, UINT_MAX}, {8, 3, 1}}; 
	int num     = 200;
	int masknum = 3;
	int ret;
	int i = 0;

	char *conffile;
	conffile = "memlink.conf";
	DINFO("config file: %s\n", conffile);

	myconfig_create(conffile);
	my_runtime_create_common("memlink");
	ht = g_runtime->ht;

	//test1 : hashtable_add_info_mask - create key
	for (i = 0; i < num; i++) {
		sprintf(key, "heihei%03d", i);
		hashtable_key_create_mask(ht, key, valuesize, maskformat, masknum, MEMLINK_LIST, 0);
	}

	for (i = 0; i < num; i++) {
		sprintf(key, "heihei%03d", i);
		HashNode* pNode = hashtable_find(ht, key);
		if (NULL == pNode) {
			DERROR("hashtable_add_info_mask error. can not find %s\n", key);
			return -1;
		}
	}

	///////test : hashtable_add_mask insert num value
	HashNode *node = NULL;
	DataBlock *dbk = NULL;
	char	 *item = NULL; 	
	int pos = 0;
	for (i = 0; i < num; i++) {
		sprintf(val, "value%03d", i);
		pos = i;
		ret = hashtable_add_mask(ht, key, val, maskarray[3], masknum, pos); //{8, 3, 1}
		if (ret < 0) {
			DERROR("add value err: %d, %s\n", ret, val);
			return ret;
		}
		
		ret = hashtable_find_value(ht, key, val, &node, &dbk, &item);
		if (ret < 0) {
			DERROR("not found value: %d, %s\n", ret, key);
			return ret;
		}
	}
	DINFO("insert %d values ok!\n", num);

	for (i = 0; i < 100; i++) {
		sprintf(val, "value%03d", i*2);
		ret = hashtable_mask(ht, key, val, maskarray[2], masknum); //{ 4, 1, UINT_MAX}
		if (MEMLINK_OK != ret) {
			DINFO("err hashtable_mask val:%s, i:%d\n", val, i);
			return ret;
		}
	}
	
	//////////test 5 :range
	unsigned int maskarray2[6][3] = { { 8, 3, 1}, {8, UINT_MAX, 1}, 
							 { UINT_MAX, 3, 1}, {4, 1, 1} , 
							 {UINT_MAX, UINT_MAX, 1},  {UINT_MAX, UINT_MAX, UINT_MAX}
						   }; 
	for (i = 0; i < 50; i++) {
		int k; 
		if (i < 25) {
			k  = 1;  //{8, UINT_MAX, 1} masknum:3		
			num = 100;
		}else{	
			k = 5;  //{UINT_MAX, UINT_MAX, UINT_MAX} masknum:0	
			num = 200;
		}
		int frompos = my_rand(num);
		int len = my_rand(num);
		int realnum;

		if (frompos < num) {
			int tmp = num - frompos;
			(len < tmp)?(realnum = len):(realnum = tmp);
		}else{
			realnum = 0;
        }
		DINFO("frompos:%d, len:%d, i:%d\n", frompos, len, i);
	
        Conn    conn;
        memset(&conn, 0, sizeof(Conn));
		ret = hashtable_range(ht, key, MEMLINK_VALUE_VISIBLE, maskarray2[k], masknum, 
			            frompos, len, &conn);
        
        if (len <= 0 && MEMLINK_ERR_RANGE_SIZE == ret) {
            DINFO("len == 0, ret error ok!\n");
            continue; 
        }

		if (MEMLINK_OK != ret) {
			DERROR("ret:%d, len:%d\n", ret, len);
			return -1;
		}

		MemLinkResult   result;
        memlink_result_parse(conn.wbuf, &result);
		
		if (result.count != realnum) {			
			DERROR("error!! realnum:%d, retnum:%d, i:%d\n", realnum, result.count, i);
			//return -1;
		}else{
			int j = frompos;
            MemLinkItem *item = result.root;

            while (item) {
                int jj;
				if (i < 25) {
					jj = j*2 + 1;
				}else{
					jj = j;
                }
				sprintf(val, "value%03d", jj);
                int ret = memcmp(item->value, val, result.valuesize);
                if (ret != 0) {
                    DERROR("error, pos:%d, val:%s\n", jj, val);
                    return -1;
                }
                j++;
                item = item->next;
            }
		}

        memlink_result_free(&result);
	}
	DINFO("hashtable range test end!\n");

	return 0;
}

