#ifndef COUNTRY_H
#define COUNTRY_H

typedef struct country{
  char *name;
  int index; // only valid for parent process, it is the index of the child that has bloomfilters/skiplists about vaccinations in this country.
  int maxFile; // only valid for monitor processes. it is the number of files in the directory that have been read so far.
} Country;

Country* create_country(const char *, int);
int isEqual_country(Country *, unsigned char *);
void readCountryFile(Country *);
void destroy_country(Country **);

#endif