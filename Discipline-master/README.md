# MyDB — Своя Реляционная СУБД

Кейс-чемпионат «Своя База»  
Авторы: Шубенина Д.В., Рыжкин А.А.

---

## Возможности СУБД

### DDL (Data Definition Language)
| Команда | Описание |
|---|---|
| `CREATE DATABASE <name>` | Создать базу данных |
| `CREATE DATABASE IF NOT EXISTS <name>` | Создать БД, если не существует |
| `DROP DATABASE <name>` | Удалить базу данных |
| `USE <name>` | Выбрать базу данных |
| `SHOW DATABASES` | Список баз данных |
| `CREATE TABLE <name> (col TYPE [constraints], ...)` | Создать таблицу |
| `CREATE TABLE IF NOT EXISTS ...` | Создать таблицу, если не существует |
| `DROP TABLE <name>` | Удалить таблицу |
| `DROP TABLE IF EXISTS <name>` | Удалить таблицу, если существует |
| `SHOW TABLES` | Список таблиц в текущей БД |

### DML (Data Manipulation Language)
| Команда | Описание |
|---|---|
| `SELECT * FROM <table>` | Выборка всех столбцов |
| `SELECT col1, col2 FROM <table>` | Проекция |
| `SELECT ... WHERE <condition>` | Выборка с фильтром |
| `INSERT INTO <table> VALUES (...)` | Вставка строки (позиционная) |
| `INSERT INTO <table> (cols) VALUES (...)` | Вставка с именами столбцов |
| `INSERT INTO <table> VALUES (...), (...)` | Вставка нескольких строк |
| `UPDATE <table> SET col=val WHERE ...` | Обновление строк |
| `DELETE FROM <table> WHERE ...` | Удаление строк |

### Типы данных
| Тип | Описание |
|---|---|
| `INTEGER` / `INT` | 64-битное целое число |
| `FLOAT` / `DOUBLE` / `REAL` | Число с плавающей точкой |
| `VARCHAR(n)` / `TEXT` | Строка |
| `BOOLEAN` / `BOOL` | Логический тип (true/false) |

### Ограничения целостности
- `PRIMARY KEY` — первичный ключ (уникальность + NOT NULL)
- `NOT NULL` — запрет NULL-значений
- `UNIQUE` — уникальность значений
- `FOREIGN KEY REFERENCES table(col)` — внешний ключ (фиксируется в схеме)

### Операторы WHERE
- `=`, `!=`, `<>`, `<`, `>`, `<=`, `>=`
- `AND`, `OR` — логические операторы
- `IS NULL`, `IS NOT NULL` — проверка NULL

---

## Архитектура

### Паттерн проектирования: **Facade (Фасад)**

Класс `DBMS` реализует паттерн «Фасад»: скрывает сложность внутренних подсистем
(`Lexer`, `Parser`, `Storage`, `Table`) за единым простым интерфейсом.

```
Клиент (main.cpp)
      │
      ▼
   DBMS (Фасад)
   ├── Parser ──► Lexer (токенизация) ──► Statement (AST)
   ├── Storage (Singleton) ──► Файловая система
   └── Table ──► Schema + Row[]
```

### Структура файлов

```
mydb/
├── include/
│   ├── Types.h         # Value, DataType — типы данных
│   ├── Schema.h        # ColumnDef, Schema — схема таблицы
│   ├── QueryResult.h   # QueryResult — результат запроса
│   ├── Table.h         # Table — таблица в памяти с CRUD
│   ├── Storage.h       # Storage (Singleton) — файловое хранилище
│   ├── Parser.h        # Lexer + Parser — SQL -> AST
│   └── DBMS.h          # DBMS (Facade) — главный класс
├── tests/
│   └── test_main.cpp   # 35 unit-тестов
├── main.cpp            # Консольный REPL
├── demo.sql            # Демонстрационный SQL
├── CMakeLists.txt      # Система сборки
└── run.sh              # Скрипт сборки и запуска
```

### Хранение данных

Данные хранятся в директории `./data/`:
```
data/
└── <database_name>/
    ├── <table_name>.schema   # Схема таблицы (текстовый формат)
    └── <table_name>.dat      # Строки таблицы (разделитель: ASCII 0x1F)
```

---

## Сборка и запуск

### Требования
- C++17 компилятор (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15+

### Сборка

```bash
# Интерактивная оболочка
./run.sh

# Запуск тестов
./run.sh --test

# Демонстрация
./run.sh --demo

# Выполнить SQL из файла
./run.sh --file my_script.sql
```

Или вручную:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
./mydb          # интерактивный режим
./mydb_test     # тесты
```

### Пример сессии

```sql
mydb> CREATE DATABASE shop;
Database created: shop

mydb> USE shop;
Using database: shop

shop> CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    name VARCHAR(50) NOT NULL,
    age INTEGER
);
Table created: users

shop> INSERT INTO users VALUES (1, 'Вася', 16);
OK (1 rows affected)

shop> SELECT * FROM users WHERE age > 10;
+----+------+-----+
| id | name | age |
+----+------+-----+
| 1  | Вася | 16  |
+----+------+-----+
1 row(s) selected.

shop> UPDATE users SET age = 17 WHERE id = 1;
OK (1 rows affected)

shop> DELETE FROM users WHERE id = 1;
OK (1 rows affected)

shop> exit
До свидания!
```

---

## Тесты

35 unit-тестов покрывают:
- CREATE/DROP DATABASE, USE, SHOW DATABASES
- CREATE/DROP TABLE (с IF NOT EXISTS / IF EXISTS), SHOW TABLES
- INSERT (позиционный, по именам столбцов, множественная вставка)
- SELECT (все столбцы, проекция, WHERE с =, >, AND)
- UPDATE + проверка изменений
- DELETE + проверка удаления
- Персистентность (перезагрузка данных)
- Ограничения PRIMARY KEY, NOT NULL
- Типы FLOAT, BOOLEAN, IS NULL
- Обработка ошибок (неизвестная таблица/столбец, синтаксическая ошибка)
