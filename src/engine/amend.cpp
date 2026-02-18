#include "engine/amend.hpp"

#include <cctype>
#include <sstream>

AmendHandler::AmendHandler(OrderBook& book)
    : m_book(book) {}

bool AmendHandler::isAlphaSymbol(const std::string& s) const {
    if (s.empty()) return false;
    for (unsigned char ch : s) {
        if (!std::isalpha(ch)) return false;
    }
    return true;
}

bool AmendHandler::isValidAmendRequest(const AmendRequest& req) const {
    if (req.orderId <= 0) return false;
    if (req.timeStamp < 0) return false;
    if (!isAlphaSymbol(req.symbol)) return false;

    // partial amend supported, ale musi zmieniać przynajmniej price albo qty
    if (!req.newPrice.has_value() && !req.newQuantity.has_value()) return false;

    if (req.newQuantity.has_value() && *req.newQuantity <= 0) return false;

    // Reguły ceny zależne od orderType:
    if (req.newPrice.has_value()) {
        if (req.orderType == domain::OrderType::Market) {
            // Market -> price musi być 0
            if (*req.newPrice != 0) return false;
        } else {
            // Limit/IOC -> price > 0
            if (*req.newPrice <= 0) return false;
        }
    }

    return true;
}

AmendResult AmendHandler::execute(const AmendRequest& req) {
    AmendResult res;
    res.orderId = req.orderId;

    // 1) walidacja samego requestu (format/typy już są poprawne – parser jutro)
    if (!isValidAmendRequest(req)) {
        res.accepted = false;
        res.rejectCode = 101;
        res.rejectMessage = "Invalid amendement details";
        return res;
    }

    // 2) znajdź istniejący order
    // Zakładamy minimalną funkcję w OrderBook: getById(orderId) -> Order*
    domain::Order* existingOrderPtr = m_book.getById(req.orderId);
    if (!existingOrderPtr) {
        res.accepted = false;
        res.rejectCode = 404;
        res.rejectMessage = "Order does not exist";
        return res;
    }

    // 3) sprawdź, czy ktoś nie próbuje zmienić pól niedozwolonych
    // Spec: wolno zmienić tylko price i/lub quantity.
    if (existingOrderPtr->symbol != req.symbol ||
        existingOrderPtr->orderType != req.orderType ||
        existingOrderPtr->side != req.side) {
        res.accepted = false;
        res.rejectCode = 101;
        res.rejectMessage = "Invalid amendement details";
        return res;
    }

    // 4) policz nowe wartości (partial amend)
    const domain::Price oldPrice = existingOrderPtr->price;
    const int oldQty = existingOrderPtr->quantity;

    const domain::Price newPrice = req.newPrice.has_value() ? *req.newPrice : oldPrice;
    const int newQty = req.newQuantity.has_value() ? *req.newQuantity : oldQty;

    // (Uwaga na przyszłość: tu wejdzie reguła matchedQuantity/filledQty.
    // Dziś możesz mieć filled=0. Jeśli dodasz filledQty do Order, to:
    // if (newQty <= existing->filledQty) => close/remove)
    if (newQty <= 0) {
        // defensywnie, choć i tak odfiltrowane wyżej
        m_book.erase(req.orderId);
        res.accepted = true;
        return res;
    }

    // 5) reguła priorytetu:
    // - tylko qty down i bez zmiany ceny -> nie tracisz priorytetu (update in place)
    const bool priceChanged = (newPrice != oldPrice);
    const bool qtyChanged   = (newQty != oldQty);
    const bool qtyDecreased = (newQty < oldQty);
    const bool onlyQtyDownNoPriceChange = (qtyChanged && qtyDecreased && !priceChanged);

    if (onlyQtyDownNoPriceChange) {
        existingOrderPtr->quantity = newQty;
        existingOrderPtr->timeStamp = req.timeStamp; // timestamp w obiekcie możesz aktualizować albo nie; spec mówi, że priorytet nie ginie
        res.accepted = true;
        return res;
    }

    // 6) wszystkie inne amendy -> mogą zmienić priorytet:
    // Najprościej: usuń i dodaj jako "nowy" (na koniec kolejki przy danym price level)
    domain::Order amended = *existingOrderPtr;
    amended.price = newPrice;
    amended.quantity = newQty;
    amended.timeStamp = req.timeStamp;

    // usuń stary wpis (z jego kolejki) i wstaw nowy
    m_book.erase(req.orderId);

    // add powinien dodać do końca FIFO na poziomie ceny i wrzucić id do liveIds
    // (jeśli erase usuwa liveIds, to add je z powrotem doda)
    const bool inserted = m_book.add(amended);
    if (!inserted) {
        // w praktyce nie powinno się zdarzyć, ale jeśli tak — to traktuj jak błąd amend
        res.accepted = false;
        res.rejectCode = 101;
        res.rejectMessage = "Invalid amendement details";
        return res;
    }

    res.accepted = true;
    return res;
}

std::string AmendHandler::format(const AmendResult& r) {
    std::ostringstream oss;
    if (r.accepted) {
        oss << r.orderId << " - AmendAccept";
    } else {
        oss << r.orderId << " - AmendReject - " << r.rejectCode << " - " << r.rejectMessage;
    }
    return oss.str();
}