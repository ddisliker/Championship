-- Демонстрация возможностей MyDB
-- Кейс-чемпионат "Своя База"

-- Создаём базу данных магазина
CREATE DATABASE shop;
USE shop;

-- DDL: Создание таблиц
CREATE TABLE products (
    id      INTEGER PRIMARY KEY,
    name    VARCHAR(100) NOT NULL,
    price   FLOAT NOT NULL,
    in_stock BOOLEAN
);

CREATE TABLE users (
    id    INTEGER PRIMARY KEY,
    name  VARCHAR(50) NOT NULL,
    age   INTEGER
);

CREATE TABLE orders (
    id         INTEGER PRIMARY KEY,
    user_id    INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    quantity   INTEGER NOT NULL
);

-- DML: Заполнение данными
INSERT INTO products VALUES (1, 'Ноутбук', 89999.99, true);
INSERT INTO products VALUES (2, 'Мышь', 1299.0, true);
INSERT INTO products VALUES (3, 'Клавиатура', 3500.50, false);
INSERT INTO products VALUES (4, 'Монитор', 45000.0, true);

INSERT INTO users VALUES (1, 'Вася', 16);
INSERT INTO users VALUES (2, 'Юля', 67);
INSERT INTO users VALUES (3, 'Аня', 25);

INSERT INTO orders VALUES (1, 1, 2, 2);
INSERT INTO orders VALUES (2, 2, 1, 1);
INSERT INTO orders VALUES (3, 3, 4, 1);

-- SELECT: Выборки

-- Все продукты
SELECT * FROM products;

-- Дорогие товары (> 5000)
SELECT name, price FROM products WHERE price > 5000.0;

-- Только товары в наличии
SELECT * FROM products WHERE in_stock = true;

-- Пользователи старше 20 лет
SELECT * FROM users WHERE age > 20;

-- Пользователи с именем не Вася
SELECT * FROM users WHERE name != 'Вася';

-- UPDATE: Обновление
UPDATE products SET price = 79999.0 WHERE id = 1;
UPDATE products SET in_stock = true WHERE id = 3;

-- Проверяем изменения
SELECT * FROM products WHERE id = 1;
SELECT * FROM products WHERE id = 3;

-- DELETE: Удаление
DELETE FROM orders WHERE id = 1;

-- Проверяем
SELECT * FROM orders;

-- SHOW: Метаданные
SHOW TABLES;
SHOW DATABASES;
