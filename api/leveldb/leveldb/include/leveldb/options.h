// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_INCLUDE_OPTIONS_H_
#define STORAGE_LEVELDB_INCLUDE_OPTIONS_H_

#include "leveldb_terark_config.h"
#if defined(HAVE_ROCKSDB) && !defined(leveldb)
#define leveldb rocksdb
#endif

#include <memory>
#include <stddef.h>
#include <terark/db/db_dll_decl.hpp>

namespace leveldb {

class Cache;
class Comparator;
class Env;
class FilterPolicy;
class Logger;
class Snapshot;

// DB contents are stored in a set of blocks, each of which holds a
// sequence of key,value pairs.  Each block may be compressed before
// being stored in a file.  The following enum describes which
// compression method (if any) is used to compress a block.
enum CompressionType {
  // NOTE: do not change the values of existing entries, as these are
  // part of the persistent format on disk.
  kNoCompression     = 0x0,
  kSnappyCompression = 0x1
#ifdef HAVE_ROCKSDB
  , kZlibCompression = 0x2
#endif
};

// Options to control the behavior of a database (passed to DB::Open)
struct TERARK_DB_DLL Options {
  // -------------------
  // Parameters that affect behavior

  // Comparator used to define the order of keys in the table.
  // Default: a comparator that uses lexicographic byte-wise ordering
  //
  // REQUIRES: The client must ensure that the comparator supplied
  // here has the same name and orders keys *exactly* the same as the
  // comparator provided to previous open calls on the same DB.
  const Comparator* comparator;

  // If true, the database will be created if it is missing.
  // Default: false
  bool create_if_missing;

  // If true, an error is raised if the database already exists.
  // Default: false
  bool error_if_exists;

  // If true, the implementation will do aggressive checking of the
  // data it is processing and will stop early if it detects any
  // errors.  This may have unforeseen ramifications: for example, a
  // corruption of one DB entry may cause a large number of entries to
  // become unreadable or for the entire DB to become unopenable.
  // Default: false
  bool paranoid_checks;

#ifdef HAVE_ROCKSDB
  // By default, RocksDB uses only one background thread for flush and
  // compaction. Calling this function will set it up such that total of
  // `total_threads` is used. Good value for `total_threads` is the number of
  // cores. You almost definitely want to call this function if your system is
  // bottlenecked by RocksDB.
  Options* IncreaseParallelism(int = 16) { return this; }
  Options* OptimizeLevelStyleCompaction() { return this; }
#endif

#if HAVE_BASHOLEVELDB
  // Riak specific: this variable replaces paranoid_checks at one
  // one place in the code.  This variable alone controls whether or not
  // compaction read operations check CRC values.  Riak needs
  // the compaction CRC check, but not other paranoid_checks ... so
  // this independent control.
  // Default: true
  bool verify_compactions;
#endif

  // Use the specified object to interact with the environment,
  // e.g. to read/write files, schedule background work, etc.
  // Default: Env::Default()
  Env* env;

  // Any internal progress/error information generated by the db will
  // be written to info_log if it is non-NULL, or to a file stored
  // in the same directory as the DB contents if info_log is NULL.
  // Default: NULL
  Logger* info_log;

  // -------------------
  // Parameters that affect performance

  // Amount of data to build up in memory (backed by an unsorted log
  // on disk) before converting to a sorted on-disk file.
  //
  // Larger values increase performance, especially during bulk loads.
  // Up to two write buffers may be held in memory at the same time,
  // so you may wish to adjust this parameter to control memory usage.
  // Also, a larger write buffer will result in a longer recovery time
  // the next time the database is opened.
  //
  // Default: 4MB
  size_t write_buffer_size;

  // Number of open files that can be used by the DB.  You may need to
  // increase this if your database has a large working set (budget
  // one open file per 2MB of working set).
  //
  // Default: 1000
  int max_open_files;

  // Control over blocks (user data is stored in a set of blocks, and
  // a block is the unit of reading from disk).

  // If non-NULL, use the specified cache for blocks.
  // If NULL, leveldb will automatically create and use an 8MB internal cache.
  // Default: NULL
  Cache* block_cache;

  // Approximate size of user data packed per block.  Note that the
  // block size specified here corresponds to uncompressed data.  The
  // actual size of the unit read from disk may be smaller if
  // compression is enabled.  This parameter can be changed dynamically.
  //
  // Default: 4K
  size_t block_size;

  // Number of keys between restart points for delta encoding of keys.
  // This parameter can be changed dynamically.  Most clients should
  // leave this parameter alone.
  //
  // Default: 16
  int block_restart_interval;

  // Compress blocks using the specified compression algorithm.  This
  // parameter can be changed dynamically.
  //
  // Default: kSnappyCompression, which gives lightweight but fast
  // compression.
  //
  // Typical speeds of kSnappyCompression on an Intel(R) Core(TM)2 2.4GHz:
  //    ~200-500MB/s compression
  //    ~400-800MB/s decompression
  // Note that these speeds are significantly faster than most
  // persistent storage speeds, and therefore it is typically never
  // worth switching to kNoCompression.  Even if the input data is
  // incompressible, the kSnappyCompression implementation will
  // efficiently detect that and will switch to uncompressed mode.
  CompressionType compression;

  // If non-NULL, use the specified filter policy to reduce disk reads.
  // Many applications will benefit from passing the result of
  // NewBloomFilterPolicy() here.
  //
  // Default: NULL
  const FilterPolicy* filter_policy;

#ifdef HAVE_HYPERLEVELDB
  // Is the database used with the Replay mechanism?  If yes, the lower bound on
  // values to compact is (somewhat) left up to the application; if no, then
  // LevelDB functions as usual, and uses snapshots to determine the lower
  // bound.  HyperLevelDB will always maintain the integrity of snapshots, so
  // the application merely has the option to hold data as if it's holding a
  // snapshot.  This just prevents compaction from grabbing data before the app
  // can get a snapshot.
  //
  // Default: false/no.
  bool manual_garbage_collection;
#endif

  // Create an Options object with default values for all fields.
  Options();
};

#ifdef HAVE_ROCKSDB
struct TERARK_DB_DLL ColumnFamilyOptions : public Options {
  ColumnFamilyOptions() : Options() {}
};

struct TERARK_DB_DLL DBOptions : public Options {
  DBOptions() : Options() {}
};
#endif

// Options that control read operations
struct TERARK_DB_DLL ReadOptions {
  // If true, all data read from underlying storage will be
  // verified against corresponding checksums.
  // Default: false
  bool verify_checksums;

  // Should the data read for this iteration be cached in memory?
  // Callers may wish to set this field to false for bulk scans.
  // Default: true
  bool fill_cache;

  // If "snapshot" is non-NULL, read as of the supplied snapshot
  // (which must belong to the DB that is being read and which must
  // not have been released).  If "snapshot" is NULL, use an impliicit
  // snapshot of the state at the beginning of this read operation.
  // Default: NULL
  const Snapshot* snapshot;

  ReadOptions()
      : verify_checksums(false),
        fill_cache(true),
        snapshot(NULL) {
  }
};

// Options that control write operations
struct TERARK_DB_DLL WriteOptions {
  // If true, the write will be flushed from the operating system
  // buffer cache (by calling WritableFile::Sync()) before the write
  // is considered complete.  If this flag is true, writes will be
  // slower.
  //
  // If this flag is false, and the machine crashes, some recent
  // writes may be lost.  Note that if it is just the process that
  // crashes (i.e., the machine does not reboot), no writes will be
  // lost even if sync==false.
  //
  // In other words, a DB write with sync==false has similar
  // crash semantics as the "write()" system call.  A DB write
  // with sync==true has similar crash semantics to a "write()"
  // system call followed by "fsync()".
  //
  // Default: false
  bool sync;

  WriteOptions()
      : sync(false) {
  }
};

#ifdef HAVE_ROCKSDB
// Options that control flush operations
struct TERARK_DB_DLL FlushOptions {
  // If true, the flush will wait until the flush is done.
  // Default: true
  bool wait;

  FlushOptions() : wait(true) {}
};
#endif

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_OPTIONS_H_