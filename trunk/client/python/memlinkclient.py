#coding: utf-8
import os, sys
from memlink import *

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

    def create(self, key, valuesize, maskstr):
        return memlink_cmd_create(self.client, key, valuesize, maskstr)

    def stat(self, key):
        mstat = MemLinkStat()
        ret = memlink_cmd_stat(self.client, key, mstat)
        if ret != MEMLINK_OK:
            mstat = None
        return ret, mstat

    def delete(self, key, value):
        return memlink_cmd_del(self.client, key, value, len(value))

    def delete_by_mask(self, key, mask):
        return memlink_cmd_del_by_mask(self.client, key, mask)

    def insert(self, key, value, maskstr, pos):
        return memlink_cmd_insert(self.client, key, value, len(value), maskstr, pos)

    def update(self, key, value, pos):
        return memlink_cmd_update(self.client, key, value, len(value), pos)

    def mask(self, key, value, maskstr):
        return memlink_cmd_mask(self.client, key, value, len(value), maskstr)

    def tag(self, key, value, tag):
        return memlink_cmd_tag(self.client, key, value, len(value), tag)

    def range(self, key, maskstr, frompos, len):
        result = MemLinkResult()
        ret = memlink_cmd_range(self.client, key, maskstr, frompos, len, result)
        if ret != MEMLINK_OK:
            result = None
        return ret, result

    def rmkey(self, key):
        return memlink_cmd_rmkey(self.client, key)

    def count(self, key, maskstr):
        result = MemLinkCount()
        ret = memlink_cmd_count(self.client, key, maskstr, result)
        if ret != MEMLINK_OK:
            return None
        return ret, result

def memlinkresult_free(self):
    memlink_result_free(self)

def memlinkresult_print(self):
    s = 'count:%d valuesize:%d masksize:%d\n' % (self.count, self.valuesize, self.masksize)
    item = self.root
    while item:
        s += 'value:%s mask:%s\n' % (item.value, repr(item.mask))
        item = item.next
    return s


MemLinkResult.close   = memlinkresult_free
MemLinkResult.__del__ = memlinkresult_free
MemLinkResult.__str__ = memlinkresult_print

def memlinkstat_print(self):
    s = 'valuesize:%d\nmasksize:%d\nblocks:%d\ndata:%d\ndata_used:%d\nmem:%d\n' % (self.valuesize, self.masksize, self.blocks, self.data, self.data_used, self.mem)

    return s

MemLinkStat.__str__ = memlinkstat_print




