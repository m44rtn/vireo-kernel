#include "FAT.h"


#define TYPE_DIR TRUE
#define TYPE_FILE FALSE


FAT32_DIR *ROOTDIR;

//TODO: redo the code entirely

static uint32_t first_data_sector;
static int total_sectors;


FAT32_BPB *GetBootCodeFAT(uint8_t drive)
{

    //Assumes ATA drive
    //uint32_t LBA = GetFirstSectLBA(drive, SYS_PATA);
    //trace("FAT BPB LBA: %i\n", LBA);
    FAT32_BPB *buf = malloc(512);
    PIO_READ_ATA(drive, 63, 1, (uint16_t *) buf);
    return buf;
}

FAT32_BPB *FATinit(uint8_t drive)
{
    BPB = GetBootCodeFAT(drive);

    uint32_t sFAT = BPB->sectFAT32;
    first_data_sector = 63 + BPB->resvSect + (BPB->nFAT * sFAT);

    total_sectors = (BPB->nSect == 0)? BPB->lnSect : BPB->nSect;

    //commented until future use.
    //trace("BPB->SectClust: %i\n", BPB->SectClust);
    //trace("BPB->clustLocRootdir: %i\n", BPB->clustLocRootdir);
    //trace("BPB->resvSect: %i\n", BPB->resvSect);
    //trace("BPB->bSector: %i\n", BPB->bSector);
    //trace("first_data_sector: %i\n", first_data_sector);
    
    
    trace("FATinit exit code: %i\n", 0);
    return BPB;
}

uint32_t *GetFile(char *filename, uint8_t drive, uint32_t dirLoc)
{
    File *file = FindFile(filename, dirLoc, drive);

    if(file->FileLoc == NULL)
    {
        error(404);
        return NULL;
    }

    uint8_t *READ = FAT32_READ_FILE(drive, file->FileLoc, file->size);

    return (uint32_t *) READ;

}

File *FindFile(char *filename, uint32_t dirLoc, uint8_t drive)
{
    //Returns the first cluster of the file
    uint32_t len = 0;
    FAT32_DIR *dir = ReadDir(drive, dirLoc, &len);
    File *file;
    char *name_holder;
    uint32_t foundFile = 0;

    uint32_t i = 0;

    for(uint32_t i = 0; i < len; i++)
    {
        //strcopy(name_holder, dir[i].name);
        /*name_holder = strtok(filename, ".");*/
        name_holder = dir[i].name;
        if(!hasStr(name_holder, filename)) continue;
        foundFile = i;
        break;
        
    }
    
    file->FileLoc = (dir[foundFile].clHi << 16) | dir[foundFile].clLo;
    file->size = dir[foundFile].fSize;

    if(foundFile == NULL)
        file->FileLoc = NULL;
    return file;
}

uint32_t FAT_Traverse(char *name)
{
    uint8_t drive;
    uint32_t cluster;
    uint32_t prev_cluster = 0;
    
    name = strtok(name , "/");
    
    if(eqlstr(name, "HD0")) drive = 0;
    else drive = 1;
    
   while(name != NULL)
   {
        name = strtok(NULL , "/");
       
        cluster = FindNextDir(name, drive, prev_cluster);
        if(cluster != 0) prev_cluster = cluster;    
   }
  
   
   /*trace("length of directory: %i\n", length);
   trace("location of directory: %i\n", dir);
   trace("last file: %s\n", dir[length].name);*/
    if(prev_cluster == 0) prev_cluster = BPB->clustLocRootdir;
    
    return prev_cluster;
    
}

File *fat_find_dir(uint8_t drive, char *DirName, uint32_t PreviousDirClust)
{
    uint32_t DIRlen = 0;
    int len = strlen(DirName);
    uint32_t res; /* = 0xFFFFFFFF;*/
    char name[8];
    File *file;
    
    strcopy(name, DirName);
    /*trace("\n\nname: %s\n", name);*/
    //get the directory
    FAT32_DIR *DIR = ReadDir(drive, 2, &DIRlen);
    
    //Take a look at all the entries in the directory and see if it contains the name of the folder
    //and check if the entry corresponds to that of a direcotry.
    //Doesn't care about the length of the name, if a part of the name is correct then it returns.
    for(int i = 0; i < DIRlen; i++) {
        if(hasStr(DIR[i].name, name) && (DIR[i].attrib & 0x10))
        {
            file->FileLoc = ((DIR[i].clHi << 16) | DIR[i].clLo);
            file->size = DIR[i].fSize;
            /*print("directory found!\n");*/
            break;
            //return file;
        }
    }

    if(res == 0x0FFFFFFF)
        return (File *) 0x0FFFFFFF;

    return file;
}

uint32_t FindNextDir(char *DirName, uint8_t drive, uint32_t prevClust)
{
    uint32_t DIRlen = 0;
    int len = strlen(DirName);
    uint32_t res; /* = 0xFFFFFFFF;*/
    char name[8];
    
    strcopy(name, DirName);
    
    
    if(hasStr(DirName, "ROOT")) return BPB->clustLocRootdir;

    //get the directory
    FAT32_DIR *DIR = ReadDir(drive, prevClust, &DIRlen);

    //Take a look at all the entries in the directory and see if it contains the name of the folder
    //and check if the entry corresponds to that of a directory.
    //Doesn't care about the length of the name, if a part of the name is correct then it returns.
    for(uint32_t i = 0; i < DIRlen; i++) 
        if(hasStr(DIR[i].name, name) && (DIR[i].attrib & 0x10))
        {
            res = i;
            break;
            i = DIRlen;
        }; 

    if(res == 0x0FFFFFFF)
        return 0x0FFFFFFF;

    return ((DIR[res].clHi << 16) | DIR[res].clLo);
}

void FAT32_WRITE_FILE(uint8_t drive, uint32_t *file, size_t size, char *name, char *path)
{
    uint32_t VFSlen = 0;
    uint32_t DTlen = 0;
    uint32_t DIRlen = 0;

    uint32_t sizeCL = (size < 4096) ? (size / 4096) * 4096 : 4096;
    tVFS *vfs = FATFindFreeClusterChain(drive, sizeCL, &VFSlen);

    //Find the directory {^<>^}
    uint32_t dirLoc = FAT_Traverse(path);
    
    //FAT32_VFS *dirTable = FAT_read_table(drive, dirLoc, &DTlen);
    FAT32_DIR *dir = ReadDir(drive, dirLoc, &DIRlen);

    //Make entry {^<>^}
    //Make NULL entry {^<>^}
    dir = FAT32_DIRPREP(drive, dir, vfs[0].cluster, size, DIRlen, name);

    //write new entry (can only handle one sector yet)
    for(int i = 0; i < (DIRlen + 2); i += (16 * BPB->SectClust))
    {
        FAT32_Write_cluster(drive, dirLoc, (uint32_t *) dir);
        //file += 512 * BPB->SectClust;
    }

    //write the dir
    PIO_WRITE_ATA(0, FAT_cluster_LBA(vfs[0].cluster), size / 512, (uint16_t *) file);

    //TODO: write entry to FAT

}

FAT32_DIR *FAT32_DIRPREP(uint8_t drive, FAT32_DIR *dir, uint32_t clust, size_t size, uint32_t len, char *name)
{
   // FAT32_DIR *Pdir = malloc((len * sizeof(FAT32_DIR)) + 32);

    //strcopy(Pdir, dir);

    dir[len].clHi = (uint16_t) clust >> 16;
    dir[len].clLo = (uint16_t) clust;
    dir[len].fSize = size;
    
    for(int i = 0; i < 8; i++)
        dir[len].name[i] = name[i];
    
    for(int i = 0; i < 3; i++)
        dir[len].ext[i] = 0x20;

    dir[len + 1].name[0] = NULL;
    
    return dir;
}

void FAT32_Write_cluster(uint8_t drive, uint32_t cluster, uint32_t *buf)
{
    PIO_WRITE_ATA(drive, FAT_cluster_LBA(cluster), BPB->SectClust, (uint16_t *) buf);
}

uint8_t *FAT32_READ_FILE(uint8_t drive, uint32_t cluster, size_t size)
{
    uint32_t nClusts = 0;
    uint32_t lba = FAT_cluster_LBA(cluster);

    uint32_t *obuf = malloc(size);
    uint16_t *file = (uint16_t *) obuf;
    PIO_READ_ATA(0, lba, ((size / 512) + 1), (uint16_t *) file);
      
    //for(int j = 0; j < 128; j++) trace("buf[i]: %i\n",  file[j]);
    //TODO: return buffer

    return (uint8_t *) obuf;
}

uint32_t FAT_cluster_LBA(uint32_t cluster)
{
    uint32_t LBA = ((cluster - 2) * BPB->SectClust) + first_data_sector;
    return LBA;
}

tVFS *FAT_read_table(uint8_t drive, uint32_t cluster, uint32_t *nClusts)
{
    tVFS *list;
    uint32_t clust = cluster;
    uint32_t table_val, iterations = 0;
    uint32_t *ftable = malloc(512);

    /*trace("cluster = %i\n", cluster);
    trace("BPB->resvSect = %i\n", BPB->resvSect);*/
    
    do
    {
        //63 has been the 'start' of the 'drive' while testing, but this should be more dynamic
        uint32_t FAT_sector = (uint32_t) 63 + BPB->resvSect + ((clust * 4) / 512);

        uint32_t entry = (clust * 4) % 512;
        PIO_READ_ATA(drive, FAT_sector, 1, (uint16_t *) ftable);
        
        table_val = *(&ftable[entry]) & 0x0FFFFFFF;

        //trace("table_val = %i\n", table_val);

        list[iterations].cluster = table_val;

        //trace("list[*(nClusts)].cluster = %i\n\n", list[iterations].cluster);
        clust = table_val;
        *(nClusts) += 1;
        iterations++;
    }while(table_val != 0 && ((table_val & 0x0FFFFFFF) < 0x0FFFFFF8));

    demalloc(ftable);

    return list;

}

FAT32_DIR *ReadDir(uint8_t drive, uint32_t cluster, uint32_t *len)
{
    //If the cluster is 0, set it to the location of the Root Directory
    if(cluster == 0) cluster = BPB->clustLocRootdir;

    uint32_t Debug_Backup_Cluster = cluster;

    //Get all the cluster chain and set the size of the buffer
    uint32_t nClusts = 0;
    tVFS *vfs = FAT_read_table(drive, cluster, &nClusts);
  
    //reserve memory for the directory's info
    uint16_t *buf = malloc(nClusts * BPB->SectClust * 512);
    uint8_t *obuf = (uint8_t *) buf;

    uint32_t DIR_len = *(len);

    if(vfs[0].cluster >= 0x0FFFFFF8) vfs[0].cluster = cluster;

    //Read all of the clusters in the cluster chain of the root directory
    for(uint32_t i = 0; i < nClusts; i++)
    { 
        if(vfs[i].cluster >= 0x0FFFFFF8) break;
          
        uint32_t lba = FAT_cluster_LBA(vfs[i].cluster);
        /*trace("Debug_Backup_Cluster = %i\n", Debug_Backup_Cluster);
        trace("vfs[i].cluster = %i\n", vfs[i].cluster);
        trace("lba = %i\n", lba);
        trace("BPB->SectClust = %i\n", BPB->SectClust);
        trace("buf = %i\n", buf);*/
        PIO_READ_ATA(0, lba, BPB->SectClust, (uint16_t *) buf);
        buf += (512 * BPB->SectClust);      
        
    }
    //Check which entry is 0, this is the last entry in the DIR
    for(int i = 0; obuf[i] != NULL; i += 32)
    {
        //trace("buffer[i]: %i\n", obuf[i]);
        DIR_len = (i/32);
    }

    FAT32_DIR *res = (FAT32_DIR *) obuf;

    *(len) = DIR_len + 1;
    return res;
}


tVFS *FATFindFreeClusterChain(uint8_t drive, uint32_t size, uint32_t *len)
{
    uint32_t nClusters = (size / 512) / BPB->SectClust;

    //reserved 10 extra clusters, it sometimes writes stuff that shouldn't be written.
    uint32_t clust = first_data_sector;// + 10;// / (BPB->SectClust * 512));

    tVFS *vfs = malloc(nClusters * sizeof(tVFS));
    uint32_t *ftable = malloc(512);

    uint32_t prev_FATsect = NULL;

    do
    {     
        uint32_t FAT_sector = (uint32_t) 63 + BPB->resvSect + ((clust * 4) / 512);
        uint32_t entry = (clust * 4) % 512;
        
        if(FAT_sector > prev_FATsect)
            PIO_READ_ATA(drive, FAT_sector, 1, (uint16_t *) ftable);

        uint32_t table_val = *(&ftable[entry]) & 0x0FFFFFFF;

        if(table_val == BPB->FSinfo) table_val = 0x0FFFFFFF;
        
        if(table_val == 0)
        {
            vfs[*(len)].cluster = clust;
            *(len) += 1;
        }
        

        clust++;
        prev_FATsect = FAT_sector;

    }while((*(len) < nClusters) && clust < (total_sectors / BPB->SectClust));

    demalloc(ftable);

    return vfs;
}


