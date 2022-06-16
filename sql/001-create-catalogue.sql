create table catalogue(
    iid serial not null primary key,
	code text not null unique,
	title text not null,
	valid_from timestamp not null default CURRENT_TIMESTAMP,
	valid_to timestamp ,
	islocal boolean not null default true,
	acomment text,
	rid_parent int references catalogue (iid),
	alevel int not null default 0
	constraint ch_dates check (valid_to is null or valid_to > valid_from)
);

create index idx_catalogue_level on catalogue(alevel);
create index idx_catalogue_parent on catalogue(rid_parent);

insert into catalogue( code, title ) values ('111', 'Fizika'), 
                                            ('222', 'Math');