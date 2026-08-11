#pragma once

#include <QString>
#include <QUuid>

namespace mcclock::utils {

inline QString generateUuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

} // namespace mcclock::utils
