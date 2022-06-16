create table books (
   iid serial not null primary key,
   rid_catalogue int not null references catalogue (iid),
   author text,
   title text, 
   year int,
   location text,
   pablisher text, 
   pages int,
   annote text,
   acomment text
);