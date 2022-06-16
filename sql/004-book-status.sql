create table status (
  iid serial not null primary key,
  code char(3) not null unique,
  title text
);

insert into status (iid, code, title)
values (-1, 'ВЫД', 'Выдано'),
       (-2, 'ПЕР', 'В переплёте'),
       (-3, 'РЕЗ', 'Зарезервировано');

alter table books
add column 
rid_status int references status (iid);