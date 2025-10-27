create table Notes(
	Id integer PRIMARY KEY NOT NULL,
	Title varchar(128) NOT NULL
);

insert into Notes values(NULL,"Test for sqlite3");