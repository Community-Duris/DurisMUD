#include <stdio.h>
#include <stdlib.h>

#define AREA_LIST	"AREA"
#define MOB_DIR		"mob"

#define ALL_MOB		"tworld.mob"

void punt(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

int main()
{
    FILE *area_list, *all_mob, *tmp_mob;
    char area[8192], buf[8192], area_name[80], mob_name[80];
    int area_count, mob_count;

    /*
     *	Open the mob files up
     */
    area_list = fopen(AREA_LIST, "r");
    if (area_list==NULL)
	punt("AREA file cannot be opened");

    all_mob = fopen(ALL_MOB, "w");
    if (all_mob==NULL)
	punt("world.mob cannot be opened");

    area_count = 0;
    mob_count = -1;
    for (;;) {
	fgets(area, 8191, area_list);
	if (area==NULL)
	    break;
	if (feof(area_list))
	    break;
	if (area[0]=='*')		/* a comment */
	    continue;
	sscanf(area, "%s", area_name);
	fprintf(stdout, "Area[%2d] : %s\n", area_count++, area_name);
	/*
	 * open up individual mob for each area
         */

	sprintf(mob_name, "%s/%s.mob", MOB_DIR, area_name);
	tmp_mob = fopen(mob_name, "r");
        if (tmp_mob==NULL) {
            fprintf(stdout, "\twarning: %s not found\n", mob_name);
        }
        else {
            for (;;) {
                fgets(buf, 8191, tmp_mob);
                if (buf==NULL)
                    break;
                if (feof(tmp_mob))
                    break;
		if (*buf=='#')
		    mob_count++;
                fputs(buf, all_mob);
            }
            fclose(tmp_mob);
        }

    }

    /*
     *	Close mob, like a good little boy
     */
    fclose(area_list);
    fclose(all_mob);

    fprintf(stdout, "\nSummary: %d mobs\n\n", mob_count);

    system("chmod 600 tworld.mob");

    fprintf(stdout, "Done\n");
}
