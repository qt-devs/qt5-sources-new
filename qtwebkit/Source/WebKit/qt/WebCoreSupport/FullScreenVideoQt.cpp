/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "FullScreenVideoQt.h"

#include "ChromeClientQt.h"
#if USE(QT_MULTIMEDIA)
#include "MediaPlayerPrivateQt.h"
#endif
#include "HTMLNames.h"
#include "HTMLVideoElement.h"
#include "Node.h"

#if USE(GSTREAMER)
#include "GStreamerGWorld.h"
#include "PlatformVideoWindowPrivate.h"
#endif

#if USE(QTKIT)
#include "QTKitFullScreenVideoHandler.h"
#endif

#if USE(QT_MULTIMEDIA)
#include <QMediaPlayer>
#endif

namespace WebCore {

#if USE(GSTREAMER) && !defined(GST_API_VERSION_1)
GStreamerFullScreenVideoHandler::GStreamerFullScreenVideoHandler()
    : m_videoElement(0)
    , m_fullScreenWidget(0)
{
}

void GStreamerFullScreenVideoHandler::setVideoElement(HTMLVideoElement* element)
{
    m_videoElement = element;
}

void GStreamerFullScreenVideoHandler::enterFullScreen()
{
    if (m_videoElement->platformMedia().type != WebCore::PlatformMedia::GStreamerGWorldType)
        return;

    GStreamerGWorld* gstreamerGWorld = m_videoElement->platformMedia().media.gstreamerGWorld;

    if (!gstreamerGWorld->enterFullscreen())
        return;

    m_fullScreenWidget = reinterpret_cast<FullScreenVideoWindow*>(gstreamerGWorld->platformVideoWindow()->window());
    m_fullScreenWidget->setVideoElement(m_videoElement);
    connect(m_fullScreenWidget, SIGNAL(closed()), this, SLOT(windowClosed()));
    m_fullScreenWidget->showFullScreen();
}

void GStreamerFullScreenVideoHandler::windowClosed()
{
    m_videoElement->exitFullscreen();
}

void GStreamerFullScreenVideoHandler::exitFullScreen()
{
    if (m_videoElement->platformMedia().type == WebCore::PlatformMedia::GStreamerGWorldType)
        m_videoElement->platformMedia().media.gstreamerGWorld->exitFullscreen();

    m_fullScreenWidget->setVideoElement(0);
    m_fullScreenWidget->close();
}
#endif

FullScreenVideoQt::FullScreenVideoQt(ChromeClientQt* chromeClient)
    : m_chromeClient(chromeClient)
    , m_videoElement(0)
{
    Q_ASSERT(m_chromeClient);

#if USE(QT_MULTIMEDIA)
    m_FullScreenVideoHandler = m_chromeClient->createFullScreenVideoHandler();
    if (m_FullScreenVideoHandler)
        connect(m_FullScreenVideoHandler, SIGNAL(fullScreenClosed()), this, SLOT(aboutToClose()));
#endif

#if USE(GSTREAMER) && !defined(GST_API_VERSION_1)
    m_FullScreenVideoHandlerGStreamer = new GStreamerFullScreenVideoHandler;
#endif

#if USE(QTKIT)
    m_FullScreenVideoHandlerQTKit = new QTKitFullScreenVideoHandler;
#endif
}

FullScreenVideoQt::~FullScreenVideoQt()
{
#if USE(QT_MULTIMEDIA)
    delete m_FullScreenVideoHandler;
#endif
#if USE(GSTREAMER) && !defined(GST_API_VERSION_1)
    delete m_FullScreenVideoHandlerGStreamer;
#endif
#if USE(QTKIT)
    delete m_FullScreenVideoHandlerQTKit;
#endif
}

void FullScreenVideoQt::enterFullScreenForNode(Node* node)
{
    Q_ASSERT(node);
    m_videoElement = static_cast<HTMLVideoElement*>(node);

#if USE(QT_MULTIMEDIA)
    Q_ASSERT(m_FullScreenVideoHandler);
    HTMLVideoElement* videoElement = static_cast<HTMLVideoElement*>(node);
    PlatformMedia platformMedia = videoElement->platformMedia();

    ASSERT(platformMedia.type == PlatformMedia::QtMediaPlayerType);
    if (platformMedia.type != PlatformMedia::QtMediaPlayerType)
        return;

    if (!m_FullScreenVideoHandler)
        return;

    MediaPlayerPrivateQt* mediaPlayerQt = mediaPlayer();
    mediaPlayerQt->removeVideoItem();
    m_FullScreenVideoHandler->enterFullScreen(mediaPlayerQt->mediaPlayer());
#endif

#if USE(GSTREAMER) && !defined(GST_API_VERSION_1)
    m_FullScreenVideoHandlerGStreamer->setVideoElement(m_videoElement);
    m_FullScreenVideoHandlerGStreamer->enterFullScreen();
#endif

#if USE(QTKIT)
    m_FullScreenVideoHandlerQTKit->enterFullScreen(m_videoElement);
#endif
}

void FullScreenVideoQt::exitFullScreenForNode(Node* node)
{
    Q_ASSERT(node);

#if USE(QT_MULTIMEDIA)
    HTMLVideoElement* videoElement = static_cast<HTMLVideoElement*>(node);
    PlatformMedia platformMedia = videoElement->platformMedia();

    ASSERT(platformMedia.type == PlatformMedia::QtMediaPlayerType);
    if (platformMedia.type != PlatformMedia::QtMediaPlayerType)
        return;

    Q_ASSERT(m_FullScreenVideoHandler);

    if (!m_FullScreenVideoHandler)
        return;

    m_FullScreenVideoHandler->exitFullScreen();
    MediaPlayerPrivateQt* mediaPlayerQt = mediaPlayer();
    mediaPlayerQt->restoreVideoItem();
#endif
#if USE(GSTREAMER) && !defined(GST_API_VERSION_1)
    m_FullScreenVideoHandlerGStreamer->exitFullScreen();
#endif

#if USE(QTKIT)
    m_FullScreenVideoHandlerQTKit->exitFullScreen();
#endif

}

void FullScreenVideoQt::aboutToClose()
{
    Q_ASSERT(m_videoElement);
    m_videoElement->exitFullscreen();
}

#if USE(QT_MULTIMEDIA)
MediaPlayerPrivateQt* FullScreenVideoQt::mediaPlayer()
{
    Q_ASSERT(m_videoElement);
    PlatformMedia platformMedia = m_videoElement->platformMedia();
    return static_cast<MediaPlayerPrivateQt*>(platformMedia.media.qtMediaPlayer);
}
#endif

bool FullScreenVideoQt::requiresFullScreenForVideoPlayback()
{
#if USE(QT_MULTIMEDIA)
    return m_FullScreenVideoHandler ? m_FullScreenVideoHandler->requiresFullScreenForVideoPlayback() : false;
#else
    return false;
#endif
}

bool FullScreenVideoQt::isValid() const
{
#if USE(QT_MULTIMEDIA)
    return m_FullScreenVideoHandler;
#endif
#if USE(GSTREAMER) && !defined(GST_API_VERSION_1)
    return m_FullScreenVideoHandlerGStreamer;
#elif USE(QTKIT)
    return m_FullScreenVideoHandlerQTKit;
#else
    return 0;
#endif
}

}

