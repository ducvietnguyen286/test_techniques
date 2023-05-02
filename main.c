#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <uthash.h>
#include <string.h>

//déclarer une structure de données Hashmap avec des champs :key,value
typedef struct {
    int key;
    char value[100];
    UT_hash_handle hh;
} HashMap;


void add_name(HashMap **hashMaps, const char *name){
	HashMap *s;

	HASH_FIND_STR(*hashMaps, name, s);
	if(s == NULL){
		s =(HashMap*) malloc(sizeof(HashMap));
		s->key = HASH_COUNT(*hashMaps);
		strcpy(s->value, name);
		s->hh.next = NULL;
		s->hh.prev =NULL;
		HASH_ADD_STR(*hashMaps, value, s);
	}
}

void create_hash_tbl(char* array_col, sqlite3 *db){
    char *ErrMsg = 0;
    int rc;
    char *sql = malloc(sizeof(char) * 100);
    char *tam = malloc(sizeof(char) * 100);
    strcpy(sql,"CREATE TABLE oa_trf_src_");
    strcat(sql, array_col);
    strcat(sql, "_lkp ");
    strcat(sql, "(id INTEGER PRIMARY KEY,champ TEXT);");
    
    printf("%s\n", sql);
    
    rc = sqlite3_exec(db, sql, 0, 0, &ErrMsg);
    if (rc != SQLITE_OK){
        fprintf(stderr, "SQL error 1: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        sqlite3_close(db);
        exit(1);
    }      
    
    free(sql);
    free(tam);
} 

void print_hashmap(HashMap * hashMaps){
	HashMap *s;
	for (s = hashMaps; s != NULL; s = s->hh.next){
		printf("key: %d, value: %s \n",s->key, s->value);
	}
}

void print_value(HashMap *HashMaps, const char *value){
	HashMap *s;
	HASH_FIND_STR(HashMaps, value, s);
	if(s != NULL){
		printf("%d : %s\n", s->key, s->value);
	}
}

HashMap* add_hashmap(char* array_col, sqlite3 *db, HashMap *HashMaps){
    int rc;
    sqlite3_stmt *stmt;
    char *sql = malloc(sizeof(char) * 100);
    char *tam = malloc(sizeof(char) * 100);
    strcpy(sql,"SELECT ");
    strcat(sql, array_col);
    strcat(sql, " FROM oa_trf_src");
    strcat(sql, " WHERE ");
    strcat(sql, array_col);
    strcat(sql, " IS NOT NULL;");
	
    printf("%s \n", sql);
    rc = sqlite3_prepare_v2(db,sql,-1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement 2: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW){
        const char *name = (const char*) sqlite3_column_text(stmt, 0);

        add_name(&HashMaps, name);
        
    }
    //print_hashmap(HashMaps);
    //print_value(HashMaps, "vld.tfnr974");
    free(sql);
    free(tam);
    
    sqlite3_finalize(stmt);
    return HashMaps;
}

//truyên hash map vào database 

void trans_hashmap_to_database(HashMap* hashMaps, sqlite3 *db, char* name){
    HashMap *s;
    int rc;
    char *sql = malloc(sizeof(char) * 100);
    char *zErrMsg = 0;
    for (s = hashMaps; s != NULL; s = s->hh.next) {
        sql = sqlite3_mprintf("INSERT INTO %s (id, champ) VALUES (%d, '%q')", name,s->key, s->value);
        rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error 2: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            sqlite3_close(db);
            exit(1);
        }
    }
}

void write_procedure(char* sql, sqlite3* db ){
    int rc ;
    char *err_msg = 0;
    rc = sqlite3_exec(db,sql,0,0,&err_msg);
    if(rc != SQLITE_OK){
        fprintf(stderr, "SQL error 3: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(1);
    }else{
        printf("l'instruction sql:%s est exécutée.\n", sql);
    }
}

int main(){
	sqlite3 *db;
	int rc;
	const char *sql;
	//HashMap *hashMaps = NULL;
	//sqlite3_stmt *stmt;
	
	//printf("test\n");

	//open database
	rc = sqlite3_open("ellipsys_test_db.db3", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }else{
    	printf("database open\n");
    }

    char *arr_col[] = {"id","trf","tgtTb","tgtLab","srcTb","srcLab"};
    char *arr_tabl_hash[] ={"oa_trf_src_id_lkp","oa_trf_src_trf_lkp","oa_trf_src_tgtTb_lkp","oa_trf_src_tgtLab_lkp","oa_trf_src_srcTb_lkp","oa_trf_src_srcLab_lkp"};
    int arrLength = sizeof(arr_col)/sizeof(arr_col[0]);
    
    for (int i = 0; i < arrLength; i++)
    {
    	HashMap *hashMaps = NULL;
    	create_hash_tbl(arr_col[i],db);	
    	hashMaps = add_hashmap(arr_col[i], db, hashMaps);
    	trans_hashmap_to_database(hashMaps, db, arr_tabl_hash[i]);
    	    	

    }
    
    write_procedure("CREATE TABLE oa_trf_src_red AS SELECT * FROM oa_trf_src;",db);
    
    write_procedure("UPDATE oa_trf_src_red SET id = (SELECT id FROM oa_trf_src_id_lkp WHERE champ = oa_trf_src_red.id) WHERE EXISTS (SELECT id FROM oa_trf_src_id_lkp WHERE champ = oa_trf_src_red.id);", db);
   	write_procedure("UPDATE oa_trf_src_red SET trf = (SELECT id FROM oa_trf_src_trf_lkp WHERE champ = oa_trf_src_red.trf) WHERE EXISTS (SELECT id FROM oa_trf_src_trf_lkp WHERE champ = oa_trf_src_red.trf);", db);
   	write_procedure("UPDATE oa_trf_src_red SET tgtTb = (SELECT id FROM oa_trf_src_tgtTb_lkp WHERE champ = oa_trf_src_red.tgtTb) WHERE EXISTS (SELECT id FROM oa_trf_src_tgtTb_lkp WHERE champ = oa_trf_src_red.tgtTb);", db);
   	write_procedure("UPDATE oa_trf_src_red SET tgtLab = (SELECT id FROM oa_trf_src_tgtLab_lkp WHERE champ = oa_trf_src_red.tgtLab) WHERE EXISTS (SELECT id FROM oa_trf_src_tgtLab_lkp WHERE champ = oa_trf_src_red.tgtLab);", db);
   	write_procedure("UPDATE oa_trf_src_red SET srcTb = (SELECT id FROM oa_trf_src_srcTb_lkp WHERE champ = oa_trf_src_red.srcTb) WHERE EXISTS (SELECT id FROM oa_trf_src_srcTb_lkp WHERE champ = oa_trf_src_red.srcTb);", db);
   	write_procedure("UPDATE oa_trf_src_red SET srcLab = (SELECT id FROM oa_trf_src_srcLab_lkp WHERE champ = oa_trf_src_red.srcLab) WHERE EXISTS (SELECT id FROM oa_trf_src_srcLab_lkp WHERE champ = oa_trf_src_red.srcLab);", db);
    
    write_procedure("CREATE TABLE  oa_trf_src_red_temp (id INTEGER,trf INTEGER,tgtTb INTEGER,tgtLab INTEGER,srcTb INTEGER,srcLab INTEGER,impact INTEGER);",db);
    write_procedure("INSERT INTO oa_trf_src_red_temp (id, trf, tgtTb,tgtLab,srcTb,srcLab,impact)SELECT CAST(id AS INTEGER), CAST(trf AS INTEGER), CAST(tgtTb AS INTEGER), CAST(tgtLab AS INTEGER), CAST(srcTb AS INTEGER), CAST(srcLab AS INTEGER), CAST(impact AS INTEGER) FROM oa_trf_src_red;",db);
    write_procedure("DROP TABLE oa_trf_src_red;",db);
    write_procedure("ALTER TABLE oa_trf_src_red_temp RENAME TO oa_trf_src_red;",db);
    //write_procedure("UPDATE oa_trf_src_red SET impact = (SELECT id FROM oa_trf_src_impact_lkp WHERE champ = oa_trf_src_red.impact) WHERE EXISTS (SELECT id FROM oa_trf_src_impact_lkp WHERE champ = oa_trf_src_red.impact);", db);
    //write_procedure("CREATE TABLE oa_trf_src_red AS SELECT * FROM oa_trf_src;",db);
   	sqlite3_close(db);

	return 0;
}