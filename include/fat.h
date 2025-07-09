#define ATTR_VOLUMELABEL	0x08
#define ATTR_ARCHIVE		0x20

typedef struct FAT_ENTRY {
	uint8_t		filename[11];
	uint8_t		attr;
	uint8_t		lcase;
	uint8_t		ctime_cs;
	uint16_t	ctime;
	uint16_t	cdate;
	uint16_t	adate;
	uint16_t	starthi;
	uint16_t	mtime;
	uint16_t	mdate;
	uint16_t	start;
	uint32_t	size;
} __attribute__((packed)) FAT_ENTRY_t;
