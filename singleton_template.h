#pragma once


template <typename T>
class Singleton {
public:
	static T *instance();
	static void destroy();
private:
	Singleton();
	~Singleton();
private:
	static T *_instance;
};

template <typename T>
T *Singleton<T>::_instance = NULL;

template <typename T>
T *Singleton<T>::instance() {
	if (_instance == NULL) {
		_instance = new T;
	}

	return _instance;
}

template <typename T>
void Singleton<T>::destroy() {
	if (_instance != NULL)
		delete _instance;
}

template <typename T>
Singleton<T>::Singleton() {
}

template <typename T>
Singleton<T>::~Singleton() {
}