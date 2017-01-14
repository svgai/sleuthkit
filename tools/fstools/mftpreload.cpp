#include "tsk/tsk_tools_i.h"
#include <locale.h>
#include <time.h>

static TSK_TCHAR *progname;

void
usage()
{
    TFPRINTF(stderr,
        _TSK_T
        ("usage: %s [-vV] [-f fstype] [-o imgoffset] image [images] [inode]\n"),
        progname);
    tsk_fprintf(stderr,
        "\tIf [inode] is not given, the root directory is used\n");
    tsk_fprintf(stderr,
        "\t-f fstype: File system type (use '-f list' for supported types)\n");
    tsk_fprintf(stderr,
        "\t-o imgoffset: Offset into image file (in sectors)\n");
    tsk_fprintf(stderr, "\t-v: verbose output to stderr\n");
    tsk_fprintf(stderr, "\t-V: Print version\n");
    exit(1);
}

int
main(int argc, char **argv1)
{
    TSK_IMG_TYPE_ENUM imgtype = TSK_IMG_TYPE_DETECT;
    TSK_IMG_INFO *img;

    TSK_OFF_T imgaddr = 0;
    TSK_FS_TYPE_ENUM fstype = TSK_FS_TYPE_DETECT;
    TSK_FS_INFO *fs;

    TSK_INUM_T inode;
    int name_flags = TSK_FS_DIR_WALK_FLAG_ALLOC | TSK_FS_DIR_WALK_FLAG_UNALLOC;
    int ch;
    extern int OPTIND;
    int fls_flags;
    int32_t sec_skew = 0;
    static TSK_TCHAR *macpre = NULL;
    TSK_TCHAR **argv;
    unsigned int ssize = 0;

#ifdef TSK_WIN32
    // On Windows, get the wide arguments (mingw doesn't support wmain)
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == NULL) {
        fprintf(stderr, "Error getting wide arguments\n");
        exit(1);
    }
#else
    argv = (TSK_TCHAR **) argv1;
#endif

    progname = argv[0];
    setlocale(LC_ALL, "");

    fls_flags = TSK_FS_FLS_DIR | TSK_FS_FLS_FILE;

    while ((ch =
            GETOPT(argc, argv, _TSK_T("ab:dDf:Fi:m:lo:prs:uvVz:"))) > 0) {
        switch (ch) {
        case _TSK_T('?'):
        default:
            TFPRINTF(stderr, _TSK_T("Invalid argument: %s\n"),
                argv[OPTIND]);
            usage();
        case _TSK_T('f'):
            if (TSTRCMP(OPTARG, _TSK_T("list")) == 0) {
                tsk_fs_type_print(stderr);
                exit(1);
            }
            fstype = tsk_fs_type_toid(OPTARG);
            if (fstype == TSK_FS_TYPE_UNSUPP) {
                TFPRINTF(stderr,
                    _TSK_T("Unsupported file system type: %s\n"), OPTARG);
                usage();
            }
            break;
        case _TSK_T('o'):
            if ((imgaddr = tsk_parse_offset(OPTARG)) == -1) {
                tsk_error_print(stderr);
                exit(1);
            }
            break;
        case _TSK_T('v'):
            tsk_verbose++;
            break;
        case _TSK_T('V'):
            tsk_version_print(stdout);
            exit(0);

        }
    }

    /* We need at least one more argument */
    if (OPTIND == argc) {
        tsk_fprintf(stderr, "Missing image name\n");
        usage();
    }

    /* open image - there is an optional inode address at the end of args 
     *
     * Check the final argument and see if it is a number
     */
	if (tsk_fs_parse_inum(argv[argc - 1], &inode, NULL, NULL, NULL, NULL)) {
		/* Not an inode at the end */
		if ((img =
			tsk_img_open(argc - OPTIND, &argv[OPTIND],
				imgtype, ssize)) == NULL) {
			tsk_error_print(stderr);
			exit(1);
		}
		if ((imgaddr * img->sector_size) >= img->size) {
			tsk_fprintf(stderr,
				"Sector offset supplied is larger than disk image (maximum: %"
				PRIu64 ")\n", img->size / img->sector_size);
			exit(1);
		}

		if ((fs = tsk_fs_open_img_remote(img, imgaddr * img->sector_size, fstype)) == NULL) {
			tsk_error_print(stderr);
			if (tsk_error_get_errno() == TSK_ERR_FS_UNSUPTYPE)
				tsk_fs_type_print(stderr);

			img->close(img);
			exit(1);
		}
	}

    fs->close(fs);
    img->close(img);

    exit(0);
}
