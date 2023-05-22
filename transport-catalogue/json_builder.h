#include "json.h"

namespace json {

class Builder {
public:
    Builder();
    
    // Объявление классов контекстов
    class KeyItemContext;
    class ValueContext;
    class DictItemContext;
    class ArrayItemContext;
    
    // Задаёт строковое значение ключа для очередной пары ключ-значение
    KeyItemContext Key(std::string key);
    // Задаёт значение, соответствующее ключу при определении словаря
    Builder& Value(Node value);
    
    // Начинает определение сложного значения-словаря
    DictItemContext StartDict();
    // Завершает определение сложного значения-словаря
    Builder& EndDict();
    
    // Начинает определение сложного значения-массива
    ArrayItemContext StartArray();
    // Завершает определение сложного значения-массива
    Builder& EndArray();
    
    // Возвращает объект json::Node, содержащий JSON, описанный предыдущими вызовами методов
    Node Build();

private:
    Node root_;
    std::vector<Node*> nodes_stack_;
    
    // Добавляет элемент в текущий узел
    template <typename T>
    void InputResult(T elem) {
        if (nodes_stack_.back()->IsArray()) {
            const_cast<Array&>(nodes_stack_.back()->AsArray()).push_back(elem);
            nodes_stack_.emplace_back(&const_cast<Array&>(nodes_stack_.back()->AsArray()).back());
        } else {
            *nodes_stack_.back() = elem;
        }
    }
};

// Класс для добавления элементов
class ItemContext {
public:
    ItemContext(Builder& builder) : builder_(builder) {}
    
    Builder::KeyItemContext Key(std::string key);
    Builder& Value(Node value);
    
    Builder::DictItemContext StartDict();
    Builder& EndDict();
    
    Builder::ArrayItemContext StartArray();
    Builder& EndArray();

private:
    Builder& builder_;
};

// Вспомогательный класс ключа
class Builder::KeyItemContext : public ItemContext {
public:
    KeyItemContext(Builder& builder) : ItemContext(builder) {}
    
    ValueContext Value(Node value);
    
    // Запрет вызова методов
    KeyItemContext Key(std::string key) = delete;
    Builder& EndDict() = delete;
    Builder& EndArray() = delete;
};

// Вспомогательный класс значения
class Builder::ValueContext : public ItemContext {
public:
    ValueContext(Builder& builder) : ItemContext(builder) {}
    
    // Запрет вызова методов
    Builder& Value(Node value) = delete;
    DictItemContext StartDict() = delete;
    ArrayItemContext StartArray() = delete;
    Builder& EndArray() = delete;
};

// Вспомогательный класс значения-словаря
class Builder::DictItemContext : public ItemContext {
public:
    DictItemContext(Builder& builder) : ItemContext(builder) {}
    
    // Запрет вызова методов
    Builder& Value(Node value) = delete;
    DictItemContext StartDict() = delete;
    ArrayItemContext StartArray() = delete;
    Builder& EndArray() = delete;
};

// Вспомогательный класс значения-массива
class Builder::ArrayItemContext : public ItemContext {
public:
    ArrayItemContext(Builder& builder) : ItemContext(builder) {}
    ArrayItemContext Value(Node value) { return ItemContext::Value(std::move(value)); }
    
    // Запрет вызова методов
    KeyItemContext Key(std::string key) = delete;
    Builder& EndDict() = delete;
};

} // end of namespace json