# Класс CCastLayer

<!-- TOC -->

- [Класс CCastLayer](#класс-ccastlayer)
    - [Настройки](#настройки)
        - [Тип выхода](#тип-выхода)
    - [Обучаемые параметры](#обучаемые-параметры)
    - [Входы](#входы)
    - [Выходы](#выходы)

<!-- /TOC -->

Класс реализует слой, преобразующий тип данных своего входа.

## Настройки

### Тип выхода

```c++
void SetOutputType( TBlobType type );
```

Устанавливает тип данных выхода.

## Обучаемые параметры

Слой не имеет обучаемых параметров.

## Входы

На единственный вход подается блоб произвольного размера с данными любого типа.

## Выходы

Единственный выход содержит блоб того же размера с данными типа `GetOutputType()`.
