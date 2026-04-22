#pragma once

#include <QMetaType>
#include <QRectF>
#include <QVector>

namespace faceveil
{
    enum class ReviewDecision
    {
        Save,
        Skip,
        CancelAll,
    };

    struct ReviewResult
    {
        ReviewDecision decision = ReviewDecision::Save;
        QVector<QRectF> finalBoxes;
    };
} // namespace faceveil

Q_DECLARE_METATYPE(faceveil::ReviewResult)
