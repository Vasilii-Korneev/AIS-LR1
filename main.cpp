#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <random>
#include <atomic>

using namespace std;

class DataItem
{
public:
    DataItem(int capacity, bool mode) : capacity(capacity), mode(mode), count(0), maxCount(0) {}

    bool Produce(int item)
    {
        unique_lock<mutex> lock(mtx);
        if (mode)
            cv.wait(lock, [this]
                    { return dataQueue.size() < capacity; });
        if (dataQueue.size() >= capacity)
        {
            cout << "   Произведен элемент № " << item << "\x1B[31m   Не добавлен в очередь, так как очередь переполнена!\033[0m" << endl;
            cv.notify_all();
            return 0;
        }
        dataQueue.push(item);
        count++;
        cout << "   Произведен элемент № " << item << "   Элементов в очереди: " << dataQueue.size() << endl;
        cv.notify_all();
        return 1;
    }

    bool Consume()
    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]
                { return !dataQueue.empty(); });
        if (dataQueue.empty())
        {
            cout << "Потребитель остановлен. Очередь пуста." << endl;
            return 0;
        }
        int item = dataQueue.front();
        dataQueue.pop();
        count--;
        cout << "   Потреблен элемент  № " << item << "   Элементов в очереди: " << dataQueue.size() << endl;
        cv.notify_all();
        return 1;
    }

    void setMaxCount(int maxCount)
    {
        this->maxCount = maxCount;
    }

    int getQueueSize()
    {
        return dataQueue.size();
    }

private:
    queue<int> dataQueue;
    int capacity;
    atomic<int> count;
    atomic<int> maxCount;
    mutex mtx;
    condition_variable cv;
    bool mode;
};

class Producer
{
public:
    Producer(DataItem &queue, int count, int pTime) : queue(queue), count(count), pTime(pTime), produced(0) {}

    void Start()
    {
        queue.setMaxCount(count);
        producerThread = thread([this]
                                {
            random_device rd;
            mt19937 gen(rd());
            uniform_real_distribution<> dis(0.0, 1.0);

            for (int i = 1; i <= count; i++) {
                queue.Produce(i);
                produced++;
                this_thread::sleep_for(chrono::milliseconds((int)(dis(gen) * pTime)));
            } });
        producerThread.detach();
    }

    void Join()
    {
        producerThread.join();
    }

    int getProducedCount()
    {
        return produced;
    }

private:
    thread producerThread;
    DataItem &queue;
    int count;
    int pTime;
    atomic<int> produced;
};

class Consumer
{
public:
    Consumer(DataItem &queue, int count, int cTime) : queue(queue), count(count), cTime(cTime), consumed(0) {}

    void Start()
    {
        consumerThread = thread([this]
                                {
            random_device rd;
            mt19937 gen(rd());
            uniform_real_distribution<> dis(0.0, 1.0);

            while (consumed < count) {
                if (queue.Consume() == -1) break;
                consumed++;
                this_thread::sleep_for(chrono::milliseconds((int)(dis(gen) * cTime)));
            } });
        consumerThread.detach();
    }

    void Join()
    {
        consumerThread.join();
    }

    int getConsumedCount()
    {
        return consumed;
    }

private:
    thread consumerThread;
    DataItem &queue;
    int count;
    int cTime;
    atomic<int> consumed;
};

int main()
{
    int count;
    int size;
    bool mode;
    int pTime;
    int cTime;

    cout << "\x1B[35m\nПотокобезопасная конкурентная очередь задач by Vasilii Korneev\033[0m\n";
    cout << "\x1B[32mСоздание потокобезопасной конкурентной очереди задач:\033[0m\n";

    while (true)
    {
        cout << "[1/5] Введите количество производимых элементов: ";
        if (!(cin >> count) || count <= 0)
        {
            cout << "\x1B[33m      Ошибка: введено некорректное значение\033[0m (нужно натуральное число).\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        break;
    }

    while (true)
    {
        cout << "[2/5] Введите размер очереди: ";
        if (!(cin >> size) || size <= 0)
        {
            cout << "\x1B[33m      Ошибка: введено некорректное значение\033[0m (нужно натуральное число).\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        break;
    }

    cout << "[3/5] Ждать освобождения очереди при её переполнении? (да/нет): ";
    string modeInput;
    cin >> modeInput;
    if (modeInput == "да")
    {
        mode = 1;
        cout << "      ОК. При переполнении очереди будет ожидаться её освобождение.\n";
    }
    else
    {
        mode = 0;
        cout << "\x1B[33m      Хммм. Тогда при переполнении очереди производимый элемент не будет в неё добавлен.\033[0m\n";
    }

    while (true)
    {
        cout << "[4/5] Введите среднее время производства элементов очереди: ";
        if (!(cin >> pTime) || pTime <= 0)
        {
            cout << "\x1B[33m      Ошибка: введено некорректное значение\033[0m (нужно натуральное число).\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        break;
    }

    while (true)
    {
        cout << "[5/5] Введите среднее время потребления элементов из очереди: ";
        if (!(cin >> cTime) || cTime <= 0)
        {
            cout << "\x1B[33m      Ошибка: введено некорректное значение\033[0m (нужно натуральное число).\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        if (cTime > pTime) cout << "\x1B[33m      Есть риск переполнения очереди, так как среднее время потребления элементов больше среднего времени их производства.\033[0m\n";
        break;
    }

    this_thread::sleep_for(chrono::milliseconds(3000));

    cout << "\n\x1B[32mПотокобезопасная конкурентная очередь задач запущена:\033[0m\n\n";

    this_thread::sleep_for(chrono::milliseconds(1000));

    DataItem dataItem(size, mode);

    Producer producer(dataItem, count, pTime);
    Consumer consumer(dataItem, count, cTime);

    producer.Start();
    consumer.Start();

    while (producer.getProducedCount() < count || dataItem.getQueueSize() > 0)
    {
        this_thread::sleep_for(chrono::milliseconds(1000));
    }

    cout << "\nПроизведено элементов:   " << producer.getProducedCount() << endl;
    cout << "Потреблено элементов:    " << consumer.getConsumedCount() << endl;
    if (producer.getProducedCount() - consumer.getConsumedCount() > 0) cout << "\x1B[33mНе потреблено элементов: \033[0m" << producer.getProducedCount() - consumer.getConsumedCount() << endl;

    cout << "\x1B[32m\nПрограмма успешно завершена.\033[0m\n\n" << endl;

    exit(0);
    return 0;
}
