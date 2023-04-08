#include "input_reader.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

using std::literals::string_literals::operator""s;

namespace transport {

void DataReader(std::istream& input, Catalogue& catalogue) {
    // Хранение строк для обработки ввода
    std::vector<std::string> stop_requests;
    // Ключ - название остановки, значение - запроса
    std::unordered_map<std::string, std::string> distance_requests;
    std::vector<std::string> bus_requests;
    
    size_t number_of_requests = 0;
    input >> number_of_requests;
    
    // Цикл по каждому запросу
    for (size_t i = 0; i < number_of_requests; ++i) {
        std::string requests;
        std::string keyword;
        
        // Чтение ключевого слова
        input >> keyword;
        
        // Чтение строки запроса
        std::getline(input, requests);
        
        // Обработка ключевых слов из запроса
        if (keyword == "Stop"s) {
            stop_requests.emplace_back(requests);
        } else if (keyword == "Bus"s) {
            bus_requests.emplace_back(requests);
        }
    }
    
    // Разбор информации об остановке из строки запроса
    for (auto& requests : stop_requests) {
        auto [stop_title, coord, query] = transport::processing::ParseStop(requests);
        
        // Сохранение строки для дальнейшей работы
        if (!query.empty()) {
            distance_requests.emplace(stop_title, query);
        }
        
        catalogue.AddStop(stop_title, coord);
    }
    
    // Разбор информации о расстоянии между остановками из строки запроса
    for (auto& [stop_title, requests] : distance_requests) {        
        processing::ParseDistance(stop_title, requests, catalogue);
    }
    
    // Разбор информации об маршруте автобуса из строки запроса
    for (auto& requests : bus_requests) {
        auto [number, stops, circular] = processing::ParseRoute(requests);
        
        catalogue.AddRoute(number, stops, circular);
    }
}

namespace processing {

/*
    Метод добавления остановки
*/

std::tuple<std::string, geo::Coordinates, std::string> ParseStop(std::string& requests) {
    // ПОлучение позиции символа после пробела
    size_t pos = requests.find_first_not_of(' ');
    
    // Извлечение названия остановки между символом пробела и двоеточием
    std::string stop_title = requests.substr(pos, requests.find(':') - pos);
    // Сдвиг позиции на длинну извлеченного слова
    pos += stop_title.length() + 2; // Включая двоеточие и символ пробела
    
    // Извлечение координаты широты между символом пробела и запятой
    std::string latit = requests.substr(pos, requests.find(',', pos) - pos);
    pos += latit.length() + 2; // Включая запятую и символ пробела
    
    // Извлечение координаты долготы
    std::string longit;
    // После координаты нет данных
    if (requests.find(',', pos) == requests.npos) {
        longit = requests.substr(pos);
        
        requests.clear();
    } else { // После координаты есть данные
        longit = requests.substr(pos, requests.find(',', pos) - pos);
        pos += longit.length() + 2; // Включая запятую и символ пробела
        // Удаляем обработанные данные для дальнейшей работы со строкой
        requests.erase(0, pos);
    }
    
    geo::Coordinates coord = { std::stod(latit), std::stod(longit) };
    
    return { stop_title, coord, requests };
}

/*
    Метод добавления маршрута
*/

std::tuple<std::string, std::vector<std::string>, bool> ParseRoute(std::string& requests) {
    // Остановки маршрута
    std::vector<std::string> stops;
    // Формат маршрута 
    bool circular = false;
    
    // Получение позиции символа после пробела
    size_t pos = requests.find_first_not_of(' ');
    // Определение признака разделения остановок
    std::string separator = (requests.find(" - ") != requests.npos) ? " - "s : " > "s;
    
    if (separator == " > "s) {
        circular = true;
    }
    
    // Извлечение названия маршрута между символом пробела и двоеточием
    std::string number = requests.substr(pos, requests.find(':') - pos);
    // Сдвиг позиции на длинну извлеченного слова
    pos += number.length() + 2; // Включая спецсимвол и символ пробела
    // Извлечение остановки маршрута
    while (pos < requests.length()) {
        std::size_t pos_next = requests.find(separator, pos);
        
        if (pos_next == requests.npos) {
            // Последняя остановка маршрута
            stops.emplace_back(requests.substr(pos));
            break;
        }
        
        stops.emplace_back(requests.substr(pos, pos_next - pos));
        pos = pos_next + separator.length();
    }
    
    return { number, stops, circular };
}

/*
    Метод добавления расстояния между остановками
*/

void ParseDistance(const std::string& stop_title, std::string& requests, Catalogue& catalogue) {
    Stop* current = const_cast<Stop*>(catalogue.FindStop(stop_title));
    Stop* next;
    int distance = 0;
    
    // Получение позиции на первый символ
    size_t pos = requests.find_first_not_of(' ');
    // Разделитель расстояния и названия
    std::string separator = "m to "s;
    
    while (!requests.empty()) {
        // Получение значения расстояния между остановками
        distance = std::stoi(requests.substr(pos, requests.find_first_of(separator)));
        
        // Удаление обработанных данных из строки
        requests.erase(pos, requests.find_first_of(separator) + separator.length());
        
        // Проверка конца строки и извлечение последних данных
        if (requests.find(separator) == requests.npos) {
            // Получение информации об остановке по извлеченному названию
            next = const_cast<Stop*>(catalogue.FindStop(requests.substr(pos)));
            
            // Добавление расстояния между остановками в справочник
            catalogue.SetStopsDistance(current, next, distance);
            
            requests.clear();
        } else {
            // Получение информации об остановке по извлеченному названию
            next = const_cast<Stop*>(catalogue.FindStop(requests.substr(pos, requests.find(','))));
            
            // Добавление расстояния между остановками в справочник
            catalogue.SetStopsDistance(current, next, distance);
            if (catalogue.GetStopsDistance(next, current) == 0) {
                catalogue.SetStopsDistance(next, current, distance);
            }
            
            // Удаление обработанных данных из строки
            requests.erase(pos, requests.find(',') + 2); // Включая пробел и запятую
        }
    }
}

} // end of namespace processing

} // end of namespace transport