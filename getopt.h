/* getopt.h
 *
 * Note that HI-TECH C does not have this function; _getopt()
 * is different; it is used to set argc and argv (and av[0]
 * with the program name).
 *
 * We take getopt.c from BSD, to avoid "reinventing the wheel".
 * This provides us with a "standard" way to scan arguments.
 */

extern int	opterr,
		optind,
		optopt;
extern char	*optarg;
int getopt(int nargc, char **nargv, char *ostr);
