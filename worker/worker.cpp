#include "worker.hpp"
#include <chrono>
#include <thread>

Worker *Worker::inst = 0;
QMutex Worker::workerLock;

void Worker::queue(WorkerContext *context) {
    QMutexLocker ml(&lock);
    context->pixmap = context->pixmap.copy();
    qqueue.enqueue(context);
}

void Worker::init() {
    QMutexLocker ml(&workerLock);
    if (!inst) inst = new Worker;
}

Worker::Worker() : QObject() {
    thr = new QThread;
    moveToThread(thr);
    connect(thr, &QThread::started, this, &Worker::process);
    connect(thr, &QThread::finished, thr, &QThread::deleteLater);
    connect(this, &Worker::finished, thr, &QThread::quit);
    connect(this, &Worker::finished, this, &Worker::deleteLater);
    connect(thr, &QThread::finished, thr, &QThread::deleteLater);
    thr->start();
}

Worker::~Worker() {
    end();
}

void Worker::end() {
    QMutexLocker ml(&endLock);
    _ended = true;
}

bool Worker::ended() {
    QMutexLocker ml(&endLock);
    return _ended;
}

void Worker::process() {
    while (!ended()) {
        lock.lock();
        if (!qqueue.isEmpty()) {
            WorkerContext *c = qqueue.dequeue();
            c->consumer(c->pixmap.toImage().convertToFormat(c->targetFormat));
        }
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // STL likes it's scopes
    }
    emit finished();
}