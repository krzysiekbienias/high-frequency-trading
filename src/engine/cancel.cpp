#include "engine/cancel.hpp"

#include <sstream>

CancelHandler::CancelHandler(OrderBook& book)
    : m_book(book) {}

bool CancelHandler::isValidCancelRequest(const CancelRequest& req) const {
    if (req.orderId <= 0) return false;
    if (req.timeStamp < 0) return false;
    return true;
}

CancelResponse CancelHandler::execute(const CancelRequest& req) {
    CancelResponse res;
    res.orderId = req.orderId;

    // 1) biznesowa walidacja requestu (bez parsowania)
    if (!isValidCancelRequest(req)) {
        res.accepted = false;
        res.rejectCode = 101;
        res.rejectMessage = "Invalid cancel details";
        return res;
    }

    // 2) czy order istnieje (live)?
    if (!m_book.isLive(req.orderId)) {
        res.accepted = false;
        res.rejectCode = 404;
        res.rejectMessage = "Order does not exist";
        return res;
    }

    // 3) usuń z booka
    (void)m_book.erase(req.orderId);

    res.accepted = true;
    return res;
}

std::string CancelHandler::format(const CancelResponse& res) {
    std::ostringstream oss;
    if (res.accepted) {
        oss << res.orderId << " - CancelAccept";
    } else {
        oss << res.orderId << " - CancelReject - " << res.rejectCode << " - " << res.rejectMessage;
    }
    return oss.str();
}