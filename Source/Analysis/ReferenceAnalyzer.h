#pragma once
#include <JuceHeader.h>
#include <functional>

namespace lab1898
{
struct ReferenceAnalysis
{
    float rmsDb = -120.0f;
    float peakDb = -120.0f;
    float lowShare = 0.33f;
    float midShare = 0.34f;
    float highShare = 0.33f;
    double analyzedSeconds = 0.0;
};

class ReferenceAnalyzer
{
public:
    static bool analyzeFile(const juce::File& file, ReferenceAnalysis& result, juce::String& error,
                            const std::function<bool()>& shouldCancel = {});
};
}
