#include "controllers/highlights/HighlightController.hpp"

#include "debug/Benchmark.hpp"

namespace {

using namespace chatterino;

auto highlightPhraseCheck(HighlightPhrase highlight) -> HighlightCheck
{
    return HighlightCheck{
        [highlight](
            const auto &args, const auto &badges, const auto &senderName,
            const auto &originalMessage) -> boost::optional<HighlightResult> {
            (void)args;             // unused
            (void)badges;           // unused
            (void)originalMessage;  // unused

            if (!highlight.isMatch(originalMessage))
            {
                return boost::none;
            }

            boost::optional<QUrl> highlightSoundUrl;
            if (highlight.hasCustomSound())
            {
                highlightSoundUrl = highlight.getSoundUrl();
            }

            return HighlightResult{
                highlight.hasAlert(),       highlight.hasSound(),
                highlightSoundUrl,          highlight.getColor(),
                highlight.showInMentions(),
            };
        }};
}

void rebuildSubscriptionHighlights(Settings &settings,
                                   std::vector<HighlightCheck> &checks)
{
    if (settings.enableSubHighlight)
    {
        auto highlightSound = settings.enableSubHighlightSound.getValue();
        auto highlightAlert = settings.enableSubHighlightTaskbar.getValue();
        auto highlightSoundUrlValue =
            settings.whisperHighlightSoundUrl.getValue();
        boost::optional<QUrl> highlightSoundUrl;
        if (!highlightSoundUrlValue.isEmpty())
        {
            highlightSoundUrl = highlightSoundUrlValue;
        }

        // The custom sub highlight color is handled in ColorProvider

        checks.emplace_back(HighlightCheck{
            [=](const auto &args, const auto &badges, const auto &senderName,
                const auto &originalMessage)
                -> boost::optional<HighlightResult> {
                (void)badges;           // unused
                (void)senderName;       // unused
                (void)originalMessage;  // unused
                if (!args.isSubscriptionMessage)
                {
                    return boost::none;
                }

                auto highlightColor =
                    ColorProvider::instance().color(ColorType::Subscription);

                return HighlightResult{
                    highlightAlert,     // alert
                    highlightSound,     // playSound
                    highlightSoundUrl,  // customSoundUrl
                    highlightColor,     // color
                    false,              // showInMentions
                };
            }});
    }
}

void rebuildWhisperHighlights(Settings &settings,
                              std::vector<HighlightCheck> &checks)
{
    if (settings.enableWhisperHighlight)
    {
        auto highlightSound = settings.enableWhisperHighlightSound.getValue();
        auto highlightAlert = settings.enableWhisperHighlightTaskbar.getValue();
        auto highlightSoundUrlValue =
            settings.whisperHighlightSoundUrl.getValue();
        boost::optional<QUrl> highlightSoundUrl;
        if (!highlightSoundUrlValue.isEmpty())
        {
            highlightSoundUrl = highlightSoundUrlValue;
        }

        // The custom whisper highlight color is handled in ColorProvider

        checks.emplace_back(HighlightCheck{
            [=](const auto &args, const auto &badges, const auto &senderName,
                const auto &originalMessage)
                -> boost::optional<HighlightResult> {
                (void)badges;           // unused
                (void)senderName;       // unused
                (void)originalMessage;  // unused
                if (!args.isReceivedWhisper)
                {
                    return boost::none;
                }

                return HighlightResult{
                    highlightAlert,
                    highlightSound,
                    highlightSoundUrl,
                    ColorProvider::instance().color(ColorType::Whisper),
                    false,
                };
            }});
    }
}

void rebuildMessageHighlights(Settings &settings,
                              std::vector<HighlightCheck> &checks)
{
    auto currentUser = getIApp()->getAccounts()->twitch.getCurrent();
    QString currentUsername = currentUser->getUserName();

    if (settings.enableSelfHighlight && !currentUsername.isEmpty())
    {
        HighlightPhrase highlight(
            currentUsername, settings.showSelfHighlightInMentions,
            settings.enableSelfHighlightTaskbar,
            settings.enableSelfHighlightSound, false, false,
            settings.selfHighlightSoundUrl.getValue(),
            ColorProvider::instance().color(ColorType::SelfHighlight));

        checks.emplace_back(highlightPhraseCheck(highlight));
    }

    auto messageHighlights = settings.highlightedMessages.readOnly();
    for (const auto &highlight : *messageHighlights)
    {
        checks.emplace_back(highlightPhraseCheck(highlight));
    }
}

void rebuildUserHighlights(Settings &settings,
                           std::vector<HighlightCheck> &checks)
{
    auto userHighlights = getCSettings().highlightedUsers.readOnly();

    for (const auto &highlight : *userHighlights)
    {
        checks.emplace_back(HighlightCheck{
            [highlight](const auto &args, const auto &badges,
                        const auto &senderName, const auto &originalMessage)
                -> boost::optional<HighlightResult> {
                (void)args;             // unused
                (void)badges;           // unused
                (void)originalMessage;  // unused

                if (!highlight.isMatch(senderName))
                {
                    return boost::none;
                }

                boost::optional<QUrl> highlightSoundUrl;
                if (highlight.hasCustomSound())
                {
                    highlightSoundUrl = highlight.getSoundUrl();
                }

                return HighlightResult{
                    highlight.hasAlert(),
                    highlight.hasSound(),
                    highlightSoundUrl,
                    highlight.getColor(),
                    false,  // showInMentions
                };
            }});
    }
}

void rebuildBadgeHighlights(Settings &settings,
                            std::vector<HighlightCheck> &checks)
{
    auto badgeHighlights = getCSettings().highlightedBadges.readOnly();

    for (const auto &highlight : *badgeHighlights)
    {
        checks.emplace_back(HighlightCheck{
            [highlight](const auto &args, const auto &badges,
                        const auto &senderName, const auto &originalMessage)
                -> boost::optional<HighlightResult> {
                (void)args;             // unused
                (void)senderName;       // unused
                (void)originalMessage;  // unused
                for (const Badge &badge : badges)
                {
                    if (highlight.isMatch(badge))
                    {
                        boost::optional<QUrl> highlightSoundUrl;
                        if (highlight.hasCustomSound())
                        {
                            highlightSoundUrl = highlight.getSoundUrl();
                        }

                        return HighlightResult{
                            highlight.hasAlert(),
                            highlight.hasSound(),
                            highlightSoundUrl,
                            highlight.getColor(),
                            false,  // showInMentions
                        };
                    }
                }

                return boost::none;
            }});
    }
}

}  // namespace

namespace chatterino {

void HighlightController::initialize(Settings &settings, Paths & /*paths*/)
{
    this->rebuildListener_.addSetting(settings.enableWhisperHighlight);
    this->rebuildListener_.addSetting(settings.enableWhisperHighlightSound);
    this->rebuildListener_.addSetting(settings.enableWhisperHighlightTaskbar);
    this->rebuildListener_.addSetting(settings.whisperHighlightSoundUrl);
    this->rebuildListener_.addSetting(settings.whisperHighlightColor);
    this->rebuildListener_.addSetting(settings.enableSelfHighlight);
    this->rebuildListener_.addSetting(settings.enableSubHighlight);
    this->rebuildListener_.addSetting(settings.enableSubHighlightSound);
    this->rebuildListener_.addSetting(settings.enableSubHighlightTaskbar);

    this->rebuildListener_.setCB([this, &settings] {
        this->rebuildChecks(settings);
    });

    this->signalHolder_.managedConnect(
        getCSettings().highlightedBadges.delayedItemsChanged,
        [this, &settings] {
            this->rebuildChecks(settings);
        });

    this->signalHolder_.managedConnect(
        getCSettings().highlightedUsers.delayedItemsChanged, [this, &settings] {
            this->rebuildChecks(settings);
        });

    this->signalHolder_.managedConnect(
        getCSettings().highlightedMessages.delayedItemsChanged,
        [this, &settings] {
            this->rebuildChecks(settings);
        });

    // this->signalHolder_.managedConnect(getApp())

    this->rebuildChecks(settings);

    // TODO: Rebuild on user changed
}

void HighlightController::rebuildChecks(Settings &settings)
{
    BenchmarkGuard benchmarkGuard("HighlightController::rebuildChecks");

    // Access checks for modification
    auto checks = this->checks_.access();
    checks->clear();

    // CURRENT ORDER:
    // Subscription -> Whisper -> User -> Message -> Badge

    rebuildSubscriptionHighlights(settings, *checks);
    qDebug() << "Rebuilt subscription highlights: " << checks->size();

    rebuildWhisperHighlights(settings, *checks);
    qDebug() << "Rebuilt whisper highlights: " << checks->size();

    rebuildUserHighlights(settings, *checks);
    qDebug() << "Rebuilt user highlights: " << checks->size();

    rebuildMessageHighlights(settings, *checks);
    qDebug() << "Rebuilt message highlights: " << checks->size();

    rebuildBadgeHighlights(settings, *checks);
    qDebug() << "Rebuilt badge highlights: " << checks->size();
}

std::pair<bool, HighlightResult> HighlightController::check(
    const MessageParseArgs &args, const std::vector<Badge> &badges,
    const QString &senderName, const QString &originalMessage) const
{
    BenchmarkGuard benchmarkGuard("HighlightController::check");

    bool highlighted = false;
    auto result = HighlightResult::emptyResult();

    // Access for checking
    const auto checks = this->checks_.accessConst();

    for (const auto &check : *checks)
    {
        if (auto checkResult =
                check.cb(args, badges, senderName, originalMessage);
            checkResult)
        {
            highlighted = true;

            if (checkResult->alert)
            {
                if (!result.alert)
                {
                    result.alert = checkResult->alert;
                }
            }

            if (checkResult->playSound)
            {
                if (!result.playSound)
                {
                    result.playSound = checkResult->playSound;
                }
            }

            if (checkResult->customSoundUrl)
            {
                if (!result.customSoundUrl)
                {
                    result.customSoundUrl = checkResult->customSoundUrl;
                }
            }

            if (checkResult->color)
            {
                if (!result.color)
                {
                    result.color = checkResult->color;
                }
            }

            if (result.full())
            {
                // The final highlight result does not have room to add any more parameters, early out
                break;
            }
        }
    }

    return {highlighted, result};
}

}  // namespace chatterino
