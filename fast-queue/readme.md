# Multiple producer multiple consumer bounded queue

В этой задаче вам предстоит реализовать одну из быстрых очередей ограниченного размера: MPMC очередь. MP означает multiple producer, т.е. возможно одновременно использовать несколько
производителей данных, которые записывают данные в очередь. MC --- multiple consumer, т.е. возможно несколько потребителей данных одновременно.

Это очень похоже на то, что уже было сделано в задаче buffered channel. Только теперь предстоит сделать более эффективную реализацию.
Реализация по умолчанию с использованием мьютекса приведена в файле `mpmc.h`. Ожидается следующее поведение:
* `Enqueue` возвращает `false` если очередь заполнена, в противном случае вставляет элемент и возвращает `true`.
* `Dequeue` возвращает `false` если очередь пуста, в противном случае достает элемент и возвращает `true`.

В отличие от реализованного ранее канала данные вызовы не блокирующие, а метода `Close` нет.

Для простоты реализации будем использовать следующий алгоритм:

1. Для того, чтобы не делать дорогостоящие операции со счетчиком, для размера очереди будем использовать только степени двойки. При этом допускается только размеры: 2, 4, 8 и т.д.
2. Создаем массив элементов нужного размера.
3. Каждый элемент массива состоит из требуемого типа T и атомарной переменной, которая указывает поколение.
4. В самом начале поколение = индексу элемента. Т.е. для нулевого элемента значение поколение = 0, для 1-го = 1 и т.д.
5. Также есть 2 индекса, которые указывают на голову и хвост очереди. При добавлении элемента увеличивается один индекс, при вытаскивании элемента из очереди - увеличивается другой счетчик.
6. Добавление элемента в очередь происходит следующим образом: если поколение == индексу, который соответствует добавлению элемента => пытаемся сделать `compare_exchange_weak` на индексе для его увеличения. Если получается => увеличиваем поколение и записываем данные.
7. Удаление элемента из очереди повторяет логику добавления: если поколение == индекс + 1, который соответствует удалению элемента => пытаемся сделать `compare_exchange_weak` на индексе для его увеличения. Если получается => в поколение записываем значение = текущая позиция + размер очереди.

### Вопросы
* Сравните реализацию по умолчанию с реализацией этого алгоритма на бенчмарках. Разница должна быть ощутимой.

### Ограничения
* Мьютексы из стандартной библиотеки запрещены в этой задаче. Это проверяется автоматически.
