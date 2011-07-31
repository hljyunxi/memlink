#coding: utf-8
import os, sys, types
from cmemlink import *

class MemLinkClietException (Exception):
    pass

class MemLinkClient:
    def __init__(self, host, readport, writeport, timeout):
        self.client = memlink_create(host, readport, writeport, timeout)
        if not self.client:
            raise MemLinkClietException

    def close(self):
        if self.client:
            memlink_close(self.client)

    def destroy(self):
        if self.client:
            memlink_destroy(self.client)
            self.client = None

    def ping(self):
        return memlink_cmd_ping(self.client)

    def dump(self):
        return memlink_cmd_dump(self.client)

    def clean(self, key):
        return memlink_cmd_clean(self.client, key)

    def create(self, key, valuesize, listtype, valuetype, attrstr=''):
        return memlink_cmd_create(self.client, key, valuesize, attrstr, listtype, valuetype)

    def create_list(self, key, valuesize, attrstr=''):
        return memlink_cmd_create_list(self.client, key, valuesize, attrstr);
    
    def create_queue(self, key, valuesize, attrstr=''):
        return memlink_cmd_create_queue(self.client, key, valuesize, attrstr);

    def create_sortlist(self, key, valuesize, valuetype, attrstr=''):
        return memlink_cmd_create_sortlist(self.client, key, valuesize, attrstr, valuetype);

    def stat(self, key):
        mstat = MemLinkStat()
        ret = memlink_cmd_stat(self.client, key, mstat)
        if ret != MEMLINK_OK:
            mstat = None
        return ret, mstat

    def stat_sys(self):
        mstat = MemLinkStatSys()
        ret = memlink_cmd_stat_sys(self.client, mstat)
        if ret != MEMLINK_OK:
            mstat = None
        return ret, mstat

    def delete(self, key, value):
        return memlink_cmd_del(self.client, key, value, len(value))

    def delete_by_attr(self, key, attr):
        return memlink_cmd_del_by_attr(self.client, key, attr)

    def insert(self, key, value, pos, attrstr=''):
        return memlink_cmd_insert(self.client, key, value, len(value), attrstr, pos)

    def sortlist_insert(self, key, value, attrstr=''):
        return memlink_cmd_sortlist_insert(self.client, key, value, len(value), attrstr)

    def sortlist_range(self, key, kind, valmin, valmax, attrstr=''):
        result = MemLinkResult()
        ret = memlink_cmd_sortlist_range(self.client, key, kind, attrstr, 
                                valmin, len(valmin), valmax, len(valmax), result)
        if ret != MEMLINK_OK:
            result = None
        return ret, result
    
    def sortlist_del(self, key, kind, valmin, valmax, attrstr=''):
        return memlink_cmd_sortlist_del(self.client, key, kind, attrstr, valmin, len(valmin), valmax, len(valmax))

    def sortlist_count(self, key, valmin, valmax, attrstr=''):
        result = MemLinkCount()
        ret = memlink_cmd_sortlist_count(self.client, key, attrstr, 
                                valmin, len(valmin), valmax, len(valmax), result)
        if ret != MEMLINK_OK:
            result = None
        return ret, result

    def move(self, key, value, pos):
        return memlink_cmd_move(self.client, key, value, len(value), pos)

    def attr(self, key, value, attrstr):
        return memlink_cmd_attr(self.client, key, value, len(value), attrstr)

    def tag(self, key, value, tag):
        return memlink_cmd_tag(self.client, key, value, len(value), tag)

    def range(self, key, kind, frompos, rlen, attrstr=''):
        result = MemLinkResult()
        ret = memlink_cmd_range(self.client, key, kind, attrstr, frompos, rlen, result)
        if ret != MEMLINK_OK:
            result = None
        return ret, result

    def rmkey(self, key):
        return memlink_cmd_rmkey(self.client, key)

    def count(self, key, attrstr=''):
        result = MemLinkCount()
        ret = memlink_cmd_count(self.client, key, attrstr, result)
        if ret != MEMLINK_OK:
            result = None
        return ret, result

    def lpush(self, key, value, attrstr=''):
        return memlink_cmd_lpush(self.client, key, value, len(value), attrstr)

    def rpush(self, key, value, attrstr=''):
        return memlink_cmd_rpush(self.client, key, value, len(value), attrstr)

    def lpop(self, key, num=1):
        result = MemLinkResult()
        ret = memlink_cmd_lpop(self.client, key, num, result)
        if ret != MEMLINK_OK:
            result = None
        return ret, result

    def rpop(self, key, num=1):
        result = MemLinkResult()
        ret = memlink_cmd_rpop(self.client, key, num, result)
        if ret != MEMLINK_OK:
            result = None
        return ret, result
    
    def read_conn_info(self):
        info = MemLinkRcInfo()
        ret = memlink_cmd_read_conn_info(self.client, info)
        if ret != MEMLINK_OK:
            info = None
        return ret, info

    def write_conn_info(self):
        info = MemLinkWcInfo()
        ret = memlink_cmd_write_conn_info(self.client, info)
        if ret != MEMLINK_OK:
            info = None
        return ret, info

    def sync_conn_info(self):
        info = MemLinkScInfo()
        ret = memlink_cmd_sync_conn_info(self.client, info)
        if ret != MEMLINK_OK:
            info = None
        return ret, info

    def insert_mkv(self, imkv):
        mkvobj = memlink_imkv_create()
        if imkv == []:
            return MEMLINK_ERR_PARAM, mkvobj
        for tup in imkv:
            if type(tup) != types.TupleType:
                return MEMLINK_ERR_PARAM, mkvobj
            if len(tup) != 4:
                return MEMLINK_ERR_PARAM, mkvobj 
            keyobj = memlink_ikey_create(tup[0], len(tup[0]))
            valobj = memlink_ival_create(str(tup[1]), len(tup[1]), str(tup[2]), int(tup[3]))
            ret = memlink_ikey_add_value(keyobj, valobj)
            ret = memlink_imkv_add_key(mkvobj, keyobj)

        ret = memlink_cmd_insert_mkv(self.client, mkvobj)
        return ret, mkvobj

def memlinkmkv_free(self):
    if MemLinkInsertMkv.is_free == 0:
        memlink_imkv_destroy(self)
        MemLinkInsertMkv.is_free = 1

MemLinkInsertMkv.close   = memlinkmkv_free
MemLinkInsertMkv.is_free = 0 
MemLinkInsertMkv.__del__ = memlinkmkv_free

def memlinkresult_list(self):
    v = []
    item = self.items
    while item:
        v.append((self.items[i].value[:self.valuesize], self.items[i].attr[:self.attrsize]))
        item = item.next
    return v

def memlinkresult_free(self):
    memlink_result_free(self)

def memlinkresult_print(self):
    s = 'count:%d valuesize:%d attrsize:%d\n' % (self.count, self.valuesize, self.attrsize)
    item = self.items
    while item:
        s += 'value:%s attr:%s\n' % (self.items[i].value, repr(self.items[i].attr))
        item = item.next
    return s

MemLinkResult.list    = memlinkresult_list
MemLinkResult.close   = memlinkresult_free
MemLinkResult.__del__ = memlinkresult_free
MemLinkResult.__str__ = memlinkresult_print

def memlinkrcinfo_free(self):
    memlink_rcinfo_free(self)

def memlinkrcinfo_print(self):
    s = 'read count: %d\n' % (self.conncount)
    item = self.root
    while item:
        s += 'fd:%s ip:%s port:%s cmd:%s conn_time:%s\n' % \
                (item.fd, item.client_ip, item.port, item.cmd_count, item.conn_time)
        item = item.next
    return s

MemLinkRcInfo.close   = memlinkrcinfo_free
MemLinkRcInfo.__del__ = memlinkrcinfo_free
MemLinkRcInfo.__str__ = memlinkrcinfo_print

def memlinkwcinfo_free(self):
    memlink_wcinfo_free(self)

def memlinkwcinfo_print(self):
    s = 'write count: %d\n' % (self.conncount)
    item = self.root
    while item:
        s += 'fd:%s ip:%s port:%s cmd:%s conn_time:%s\n' % \
                (item.fd, item.client_ip, item.port, item.cmd_count, item.conn_time)
        item = item.next
    return s

MemLinkWcInfo.close   = memlinkwcinfo_free
MemLinkWcInfo.__del__ = memlinkwcinfo_free
MemLinkWcInfo.__str__ = memlinkwcinfo_print

def memlinkscinfo_free(self):
    memlink_scinfo_free(self)

def memlinkscinfo_print(self):
    s = 'slave count: %d\n' % (self.conncount)
    item = self.root
    while item:
        s += 'fd:%s ip:%s port:%s cmd:%s conn_time:%s logver:%s logline:%s delay:%s\n' % \
                (item.fd, item.client_ip, item.port, item.cmd_count, item.conn_time, item.logver, item.logline, item.delay)
        item = item.next
    return s

MemLinkScInfo.close   = memlinkscinfo_free
MemLinkScInfo.__del__ = memlinkscinfo_free
MemLinkScInfo.__str__ = memlinkscinfo_print


def memlinkstat_print(self):
    s = 'valuesize:%d\nattrsize:%d\nblocks:%d\ndata:%d\ndata_used:%d\nmem:%d\n' % \
        (self.valuesize, self.attrsize, self.blocks, self.data, self.data_used, self.mem)

    return s

MemLinkStat.__str__ = memlinkstat_print

def memlinkstatsys_print(self):
    s = 'keys:%d\nvalues:%d\nblocks:%d\ndata_all:%d\nht_mem:%d\npool.mem:%d\npool_blocks:%d\nall_mem:%d\nlogver:%d\nlogline:%d\n' % \
        (self.keys, self.values, self.blocks, self.data_all, self.ht_mem, self.pool_mem, self.pool_blocks, self.all_mem, self.logver, self.logline)

    return s

MemLinkStatSys.__str__ = memlinkstatsys_print

