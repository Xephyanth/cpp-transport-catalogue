#include "json_builder.h"

#include <utility>

using namespace json;

Builder::Builder() {
    // Помещение корневого узла в стек
    nodes_stack_.emplace_back(&root_);
}

Builder::KeyItemContext Builder::Key(std::string key) {
    // Проверка, что стек не пустой и верхний узел - словарь
    if (!(!nodes_stack_.empty() && nodes_stack_.back()->IsDict())) {
        throw std::logic_error("Key outside the dictionary");
    }
    
    // Создание нового узла с ключем
    nodes_stack_.emplace_back(&const_cast<Dict&>(nodes_stack_.back()->AsDict())[std::move(key)]);
    
    return *this;
}

Builder& Builder::Value(Node value) {
    // Проверка, что стек не пустой и верхний узел не - NULL или массив
    if (nodes_stack_.empty() || (!nodes_stack_.back()->IsNull() && !nodes_stack_.back()->IsArray())) {
        throw std::logic_error("Value error");
    }
    
    // Добавление нового элемента в конец массива или замена значения верхнего узла
    if (nodes_stack_.back()->IsArray()) {
        const_cast<Array&>(nodes_stack_.back()->AsArray()).emplace_back(std::move(value));
    } else {
        *nodes_stack_.back() = std::move(value);
        
        nodes_stack_.pop_back();
    }
    
    return *this;
}

Builder::DictItemContext Builder::StartDict() {
    // Проверка, что стек не пустой и верхний узел не - NULL или массив
    if (nodes_stack_.empty() || (!nodes_stack_.back()->IsNull() && !nodes_stack_.back()->IsArray())) {
        throw std::logic_error("StartDict error");
    }
    
    // Добавление нового словаря в текущий словарь/массив
    InputResult(Dict());
    
    return *this;
}

Builder::ArrayItemContext Builder::StartArray() {
    // Проверка, что стек не пустой и верхний узел не - NULL или массив
    if (nodes_stack_.empty() || (!nodes_stack_.back()->IsNull() && !nodes_stack_.back()->IsArray())) {
        throw std::logic_error("StartArray error");
    }
    
    // Добавление нового массива в текущий словарь/массив
    InputResult(Array());
    
    return *this;
}

Builder& Builder::EndDict() {
    // Проверка, что стек не пустой и верхний узел - словарь
    if (!(!nodes_stack_.empty() && nodes_stack_.back()->IsDict())) {
        throw std::logic_error("EndDict outside the dictionary");
    }
    
    // Удаление верхнего узла
    nodes_stack_.pop_back();
    
    return *this;
}

Builder& Builder::EndArray() {
    // Проверка, что стек не пустой и верхний узел - массив
    if (!(!nodes_stack_.empty() && nodes_stack_.back()->IsArray())) {
        throw std::logic_error("EndArray outside the array");
    }
    
    // Удаление верхнего узла
    nodes_stack_.pop_back();
    
    return *this;
}

Node Builder::Build() {
    // Проверка, что стек не пустой
    if (!nodes_stack_.empty()) {
        throw std::logic_error("Object haven't build");
    }
    
    // Возврат корневого узла
    return root_;
}

// Добавление ключа
Builder::KeyItemContext ItemContext::Key(std::string key) {
    return builder_.Key(std::move(key));
}

// Добавление значения
Builder& ItemContext::Value(Node value) {
    return builder_.Value(std::move(value));
}

// СОздание нового словаря
Builder::DictItemContext ItemContext::StartDict() {
    return builder_.StartDict();
}

// Создание нового массива
Builder::ArrayItemContext ItemContext::StartArray() {
    return builder_.StartArray();
}

// Завершение словаря
Builder& ItemContext::EndDict() {
    return builder_.EndDict();
}

// Завершение массива
Builder& ItemContext::EndArray() {
    return builder_.EndArray();
}

// Добавление значения в словарь
Builder::ValueContext Builder::KeyItemContext::Value(Node value) {
    return ItemContext::Value(std::move(value));
}