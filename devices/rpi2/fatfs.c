//static struct fs* fat_fs;

#include "fat/ff.h"

static FATFS fat_fs;

/*void vfs_register(struct fs *fs) {
  printf("~~ vfs_register: %s/%s (%p) block_size: %d\r\n",fs->parent->device_name,fs->fs_name,fs,fs->block_size);
  printf("~~ read_directory: %p fopen: %p\r\n",fs->read_directory,fs->fopen);

  //char* name = "/";
  //struct dirent* dir = fs->read_directory(fs,&name);
  //printf("~~ dirent: %p name: %s\r\n",dir,dir->name);

  fat_fs = fs;
}*/

static Cell* _fatfs_stream;

void fatfs_debug() {
  //printf("[fatfs_debug] fs: %p read_directory: %p\r\n",fat_fs,fat_fs->read_directory);
}

Cell* fatfs_open(Cell* cpath) {
  printf("[fatfs_open] called\r\n");
  if (!cpath || cpath->tag!=TAG_STR) {
    printf("[fatfs_open] error: non-string path given\r\n");
    _fatfs_stream = alloc_string_copy("404");
    return alloc_nil();
  }

  char* path = cpath->addr;
  //struct fs* fat_fs = &fat_fs_;
  
  if (!strncmp(path,"/sd/",4)) {
    char* name = NULL;
    /*printf("[fatfs] about to read_directory… %p (%p)\r\n",fat_fs,fat_fs->read_directory);
    if (!fat_fs->read_directory) {
      printf("[fatfs] fatal error, fat_fs->read_directory is null.\r\n");
      return NULL;
    }*/
    //struct dirent* dir = fat_fs->read_directory(fat_fs,&name);

    char* filename = NULL;
    if (strlen(path)>4) {
      filename = path+4;
    }
    
    //printf("~~ dirent: %p name: %s\r\n",dir,dir->name);

    if (filename) {
      // look for the file
      printf("FAT looking for %s...\r\n",filename);
      /*while (dir) {
        if (!strcmp(filename, dir->name)) {
          // found it
          printf("FAT found file. opening...\r\n");
          fs_file* f = fat_fs->fopen(fat_fs, dir, "r");
          if (f) {
            printf("FAT trying to read file of len %d...\r\n",f->len);
            Cell* res = alloc_num_bytes(f->len);
            int len = fat_fs->fread(fat_fs, res->addr, f->len, f);
            printf("FAT bytes read: %d\r\n",len);
            // TODO: close?
            _fatfs_stream = res;
            return res;
          } else {
            // TODO should return error
            printf("FAT could not open file :(\r\n");
            _fatfs_stream = alloc_string_copy("<error: couldn't open file.>"); // FIXME hack
            return _fatfs_stream;
          }
        }
        dir = dir->next;
        }*/
      _fatfs_stream = alloc_string_copy("<error: file not found.>");

      FILINFO nfo;
      FRESULT rc = f_stat(filename, &nfo);
      if (rc) {
        printf("Failed to stat file %s: %u\r\n", filename, rc);
        return _fatfs_stream;
      }
      
      FIL fp;
      rc = f_open(&fp, filename, FA_READ);
      if (rc) {
        printf("Failed to open file %s: %u\r\n", filename, rc);
        return _fatfs_stream;
      }

      printf("filesize: %d\r\n",nfo.fsize);
      
      uint32_t buf_sz = nfo.fsize;
      _fatfs_stream = alloc_num_bytes(buf_sz+1);
      UINT bytes_read;

      rc = f_read(&fp, _fatfs_stream->addr, buf_sz, &bytes_read);
      if (rc) printf("Read failed: %u\r\n", rc);
      
      rc = f_close(&fp);
      
      return _fatfs_stream;
    } else {
      // directory
      
      Cell* res = alloc_num_string(4096);
      char* ptr = (char*)res->addr;

      FRESULT rc;
      DIR dj;			/* Pointer to the open directory object */
      FILINFO fno;

      rc = f_opendir(&dj, "/");

      printf("opendir: %d\r\n",rc);

      if (!rc) do {
        rc = f_readdir(&dj, &fno);
        printf("file: %s\r\n",fno.fname);
        int len = sprintf(ptr,"%s",fno.fname);
        ptr[len] = '\n';
        ptr+=len+1;
      } while (!rc && dj.sect>0);
      _fatfs_stream = res; // FIXME hack
      return res;
    }
  }

  Cell* result_cell = alloc_int(0);
  return result_cell;
}

Cell* fatfs_read(Cell* stream) {
  return alloc_clone(_fatfs_stream);
}

Cell* fatfs_write(Cell* stream, Cell* packet) {
  Stream* s = (Stream*)stream->addr;
  FIL fp;
  char* path = ((char*)s->path->addr)+3;
  printf("writing to stream with path %s\r\n",path);
  FRESULT rc = f_open(&fp, path, FA_WRITE|FA_CREATE_NEW);
  UINT bytes_written = 0;
  if (!rc) {
    printf("opened for writing!\r\n");
    rc = f_write(&fp, packet->addr, packet->size, &bytes_written);
    printf("rc: %d bytes_written: %d\r\n",rc,bytes_written);
    rc = f_close(&fp);
  }
  return alloc_int(bytes_written);
}

Cell* fatfs_mmap(Cell* stream) {
  return _fatfs_stream;
}
  
void mount_fatfs() {
  f_mount(0, &fat_fs);
  fs_mount_builtin("/sd", fatfs_open, fatfs_read, fatfs_write, 0, fatfs_mmap);
}

