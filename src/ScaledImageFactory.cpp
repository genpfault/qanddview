#include "ScaledImageFactory.h"

#include <wx/mstream.h>

#include <memory>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>

using namespace std;


const unsigned char background_png[] =
{
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x91, 0x68,
    0x36, 0x00, 0x00, 0x00, 0x01, 0x73, 0x52, 0x47, 0x42, 0x00, 0xAE, 0xCE, 0x1C, 0xE9, 0x00, 0x00,
    0x00, 0x04, 0x67, 0x41, 0x4D, 0x41, 0x00, 0x00, 0xB1, 0x8F, 0x0B, 0xFC, 0x61, 0x05, 0x00, 0x00,
    0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0E, 0xC3, 0x00, 0x00, 0x0E, 0xC3, 0x01, 0xC7,
    0x6F, 0xA8, 0x64, 0x00, 0x00, 0x00, 0x2B, 0x49, 0x44, 0x41, 0x54, 0x38, 0x4F, 0x63, 0xD8, 0x8F,
    0x03, 0xFC, 0xC7, 0x01, 0x46, 0x35, 0x20, 0x03, 0xA8, 0x3C, 0x06, 0x20, 0x5D, 0x03, 0x94, 0xC6,
    0x00, 0x50, 0x7D, 0x18, 0x60, 0x54, 0x03, 0x32, 0x80, 0xCA, 0x63, 0x00, 0x12, 0x35, 0xEC, 0xDF,
    0x0F, 0x00, 0xA7, 0x10, 0x9D, 0x1F, 0xC1, 0x17, 0x37, 0x45, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
    0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};


// blends a foreground RGB triplet (fg) onto a background RGB triplet (bg)
// using the given alpha value; returns the blended result
// http://stackoverflow.com/questions/12011081/alpha-blending-2-rgba-colors-in-c/12016968#12016968
inline void BlendRgb
    (
    unsigned char* dst,
    const unsigned char* fg,
    const unsigned char* bg,
    const unsigned char alpha
    )
{
    const unsigned int intAlpha = alpha + 1;
    const unsigned int invAlpha = 256 - alpha;
    for( size_t i = 0; i < 3; ++i )
    {
        dst[i] = ( ( intAlpha * fg[i] + invAlpha * bg[i] ) >> 8 );
    }
}


// blends fg onto a repeating pattern of bg into dst
// bg dimensions must be powers-of-two
void BlendPattern
    (
    wxImage& dst,
    const wxImage& fg,
    const wxImage& bg
    )
{
    const size_t fgW = static_cast< size_t >( fg.GetWidth() );
    const size_t dstW = static_cast< size_t >( dst.GetWidth() );
    const size_t dstH = static_cast< size_t >( dst.GetHeight() );

    const size_t bgW = static_cast< size_t >( bg.GetWidth() );
    const size_t bgH = static_cast< size_t >( bg.GetHeight() );

    const unsigned char* fgData = fg.GetData();
    const unsigned char* fgAlpha = fg.GetAlpha();
    unsigned char* bgData = bg.GetData();
    unsigned char* dstData = dst.GetData();

    for( size_t y = 0; y < dstH; ++y )
    {
        unsigned char* dstPx = &dstData[ y * dstW * 3 ];

        const unsigned char* fgPx = &fgData[ y * fgW * 3 ];
        const unsigned char* fgAlphaPx = &fgAlpha[ y * fgW ];

        const size_t bgY = ( y & ( bgH-1 ) ) * bgW * 3;
        const unsigned char* bgRow = &bgData[ bgY ];

        for( size_t x = 0; x < dstW; ++x )
        {
            const size_t bgX = ( x & ( bgW-1 ) ) * 3;
            const unsigned char* bgPx = &bgRow[ bgX ];

            BlendRgb( dstPx, fgPx, bgPx, *fgAlphaPx );

            dstPx += 3;
            fgPx += 3;
            fgAlphaPx += 1;
        }
    }
}


void GetScaledSubrect( wxImage& dst, const wxImage& src, const double scale, const wxPoint& pos, const int filter )
{
    if( filter == -1 )
    {
        const size_t srcW = static_cast< size_t >( src.GetWidth() );
        const size_t dstW = static_cast< size_t >( dst.GetWidth() );
        const size_t dstH = static_cast< size_t >( dst.GetHeight() );

        const float scaleInv = 1.0f / scale;

        // color
        {
            const unsigned char* srcData = src.GetData();
            unsigned char* dstData = dst.GetData();

            for( size_t dstY = 0; dstY < dstH; ++dstY )
            {
                unsigned char* dstRow = &dstData[ dstY * dstW * 3 ];
    
                const size_t srcY( ( dstY + pos.y ) * scaleInv );
                const unsigned char* srcRow = &srcData[ srcY * srcW * 3 ];

                for( size_t dstX = 0; dstX < dstW; ++dstX )
                {
                    const size_t srcX( ( dstX + pos.x ) * scaleInv );
                    const unsigned char* srcPx = &srcRow[ srcX * 3 ];
                    dstRow[ dstX * 3 + 0 ] = srcPx[ 0 ];
                    dstRow[ dstX * 3 + 1 ] = srcPx[ 1 ];
                    dstRow[ dstX * 3 + 2 ] = srcPx[ 2 ];
                }
            }
        }

        if( !src.HasAlpha() )
            return;

        // alpha
        {
            const unsigned char* srcData = src.GetAlpha();
            unsigned char* dstData = dst.GetAlpha();

            for( size_t dstY = 0; dstY < dstH; ++dstY )
            {
                unsigned char* dstRow = &dstData[ dstY * dstW ];
    
                const size_t srcY( ( dstY + pos.y ) * scaleInv );
                const unsigned char* srcRow = &srcData[ srcY * srcW ];

                for( size_t dstX = 0; dstX < dstW; ++dstX )
                {
                    const size_t srcX( ( dstX + pos.x ) * scaleInv );
                    const unsigned char* srcPx = &srcRow[ srcX ];
                    dstRow[ dstX + 0 ] = srcPx[ 0 ];
                }
            }
        }
    }
    else
    {
        const stbir_filter filter = STBIR_FILTER_TRIANGLE;
        const stbir_edge edge = STBIR_EDGE_CLAMP;
        const stbir_colorspace colorspace = STBIR_COLORSPACE_SRGB;

        stbir_resize_subpixel
            (
            src.GetData(), src.GetWidth(), src.GetHeight(), 0,
            dst.GetData(), dst.GetWidth(), dst.GetHeight(), 0,
            STBIR_TYPE_UINT8,
            3,
            0,
            STBIR_ALPHA_CHANNEL_NONE,
            edge, edge,
            filter, filter,
            colorspace,
            NULL,
            scale, scale,
            static_cast< float >( pos.x ), static_cast< float >( pos.y )
            );

        if( !src.HasAlpha() )
            return;

        stbir_resize_subpixel
            (
            src.GetAlpha(), src.GetWidth(), src.GetHeight(), 0,
            dst.GetAlpha(), dst.GetWidth(), dst.GetHeight(), 0,
            STBIR_TYPE_UINT8,
            1,
            0,
            STBIR_FLAG_ALPHA_PREMULTIPLIED,
            edge, edge,
            filter, filter,
            colorspace,
            NULL,
            scale, scale,
            static_cast< float >( pos.x ), static_cast< float >( pos.y )
            );
    }
}

// threadland
wxThread::ExitCode ScaledImageFactory::Entry()
{
    JobItem job;
    while( wxSORTABLEMSGQUEUE_NO_ERROR == mJobPool.Receive( job ) )
    {
        if( NULL == job.second.mImage || wxThread::This()->TestDestroy() )
            break;

        const ExtRect& rect = job.first;
        Context& ctx = job.second;

        ResultItem result;
        result.mGeneration = ctx.mGeneration;
        result.mRect = rect;

        // skip this rect if it isn't currently visible
        {
            wxCriticalSectionLocker locker( mVisibleCs );
            if( !mVisible.Intersects( get<2>( rect ) ) )
            {
                mResultQueue.Post( result );
                continue;
            }
        }

        wxImagePtr temp( new wxImage( get<2>( rect ).GetSize(), false ) );
        if( ctx.mImage->HasAlpha() )
        {
            temp->SetAlpha( NULL );
        }

        GetScaledSubrect
            (
            *temp,
            *ctx.mImage,
            ctx.mScale,
            get<2>( rect ).GetPosition(),
            get<1>( rect )
            );

        if( ctx.mImage->HasAlpha() )
        {
            result.mImage = new wxImage( get<2>( rect ).GetSize(), false );
            BlendPattern( *result.mImage, *temp, mStipple );
        }
        else
        {
            result.mImage = temp;
        }

        mResultQueue.Post( result );

        wxQueueEvent( mEventSink, new wxThreadEvent( wxEVT_THREAD, mEventId ) );
    }

    return static_cast< wxThread::ExitCode >( 0 );
}

ScaledImageFactory::ScaledImageFactory( wxEvtHandler* eventSink, int id )
    : mEventSink( eventSink ), mEventId( id )
{
    size_t numThreads = wxThread::GetCPUCount();
    if( numThreads <= 0 )   numThreads = 1;
    if( numThreads > 1 )    numThreads--;

    for( size_t i = 0; i < numThreads; ++i )
    {
        CreateThread();
    }

    for( wxThread*& thread : GetThreads() )
    {
        if( NULL == thread )
            continue;

        if( thread->Run() != wxTHREAD_NO_ERROR )
        {
            delete thread;
            thread = NULL;
        }
    }
    
    Reset();

    wxMemoryInputStream memStream( background_png, sizeof( background_png ) );
    mStipple.LoadFile( memStream );
}

ScaledImageFactory::~ScaledImageFactory()
{
    // clear job queue and send down "kill" jobs
    mJobPool.Clear();
    for( size_t i = 0; i < GetThreads().size(); ++i )
    {
        mJobPool.Post( JobItem( ExtRect(), Context() ) );
    }

    for( wxThread* thread : GetThreads() )
    {
        if( NULL == thread )
            continue;

        thread->Wait();
    }
}

void ScaledImageFactory::SetImage( wxImagePtr& newImage )
{
    if( NULL == newImage )
        throw std::runtime_error( "Image not set!" );

    mCurrentCtx.mImage = newImage;
    mJobPool.Clear();
}

void ScaledImageFactory::SetScale( double newScale )
{
    if( NULL == mCurrentCtx.mImage )
        throw std::runtime_error( "Image not set!" );

    mCurrentCtx.mGeneration++;
    mCurrentCtx.mScale = newScale;
    mJobPool.Clear();
}

bool ScaledImageFactory::AddRect( const ExtRect& rect )
{
    if( NULL == mCurrentCtx.mImage )
        throw std::runtime_error( "Image not set!" );

    return( wxSORTABLEMSGQUEUE_NO_ERROR == mJobPool.Post( JobItem( rect, mCurrentCtx ) ) );
}

bool ScaledImageFactory::GetImage( ExtRect& rect, wxImagePtr& image )
{
    ResultItem item;
    wxMessageQueueError err;
    while( true )
    {
        err = mResultQueue.ReceiveTimeout( 0, item );
        if( wxMSGQUEUE_TIMEOUT == err )
            return false;
        if( wxMSGQUEUE_MISC_ERROR == err )
            throw std::runtime_error( "ResultQueue misc error!" );
        if( item.mGeneration != mCurrentCtx.mGeneration )
            continue;
        break;
    }

    rect = item.mRect;
    image = item.mImage;
    return true;
}

void ScaledImageFactory::SetVisibleArea( const wxRect& visible )
{
    wxCriticalSectionLocker locker( mVisibleCs );
    mVisible = visible;
}

void ScaledImageFactory::Reset()
{
    mJobPool.Clear();
    mResultQueue.Clear();

    mCurrentCtx.mGeneration++;
    mCurrentCtx.mScale = 1.0;
    mCurrentCtx.mImage.reset();
}