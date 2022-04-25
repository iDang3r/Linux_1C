// By Alex Rusin

#ifndef CATALOG_H
#define CATALOG_H

#define CATALOG_MAX_LEN 63

extern void my_sayHello(void);

#define CATALOG_GET "GET"
#define CATALOG_ADD "ADD"
#define CATALOG_DEL "DEL"

typedef struct {
    char str[CATALOG_MAX_LEN + 1];
    unsigned int len;
} StrCatalog;

typedef struct {
    StrCatalog name;
    StrCatalog surname;
    uint32_t   age;
    StrCatalog phone;
} UserCatalog;

extern long catalog_get_user(const char* surname, unsigned int len, UserCatalog* output_data);
extern long catalog_add_user(UserCatalog* input_data);
extern long catalog_del_user(const char* surname, unsigned int len);

#endif // CATALOG_H
