// On-disk file system format.
// Both the kernel and user programs use this header file.

// Block 0 is unused.
// Block 1 is super block.
// Blocks 2 through sb.ninodes/IPB hold inodes.
// Then free bitmap blocks holding sb.size bits.
// Then sb.nblocks data blocks.
// Then sb.nlog log blocks.

#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size

// File system super block
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
};

// each inode has:
//  10 direct block pointers
//  2 singly-indirect block pointers
//  1 doubly-indirect block pointer

#define NUM_DIRECT 10
#define NUM_INDIRECT 2
#define NUM_DOUBLE_INDIRECT 1

#define PTRS_PER_BLOCK (BSIZE / sizeof(uint))

// num of blocks addressed by singly-indirect pointers
#define NINDIRECT (NUM_INDIRECT * PTRS_PER_BLOCK)
// num of blocks address by doubly-indirect pointers
#define NDOUBLE (NUM_DOUBLE_INDIRECT * PTRS_PER_BLOCK * PTRS_PER_BLOCK)
// maximum number of blocks a file can be
#define MAXFILE (NUM_DIRECT + NINDIRECT + NDOUBLE)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  // number of data block addresses
  uint addrs[NUM_DIRECT + NUM_INDIRECT + NUM_DOUBLE_INDIRECT];
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i)     ((i) / IPB + 2)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block containing bit for block b
#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};
