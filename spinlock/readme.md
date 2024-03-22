# SpinLock

В этой задаче вам предстоит реализовать spinlock --- аналог мьютекса, реализованный только с помощью атомиков и поэтому не использующий средства операционной системы для
блокирования.

Реализация по умолчанию с мьютексом приведена в файле spinlock.h. Вам необходимо переписать её без использования мьютекса. Для этого вам понадобится атомарный флаг (взят ли лок), а само взятие
блокировки будет осуществляться примерно так:

1. Проверить взят ли флаг и если нет, то взять его.
2. Крутиться в цикле, пока не удастся выполнить пункт 1.

Важно, что операция 1 должна быть атомарной.

### Вопросы

* Быстрее ли такая реализация мьютекса, чем реализация по умолчанию?
* Попробуйте добавить в цикл ожидания вызов `std::this_thread::yield()`. Изменилось ли время выполнения? Почему?

### Полезные ссылки
* https://en.cppreference.com/w/cpp/atomic/atomic_flag

### Ограничения
* Мьютексы и sleep из стандартной библиотеки запрещены в этой задаче. Это проверяется автоматически.
* Время выполнения каждого бенчмарка не должно превышать 45 мс.