#include "gerror-wrapper.h"


GerrorWrapper::GerrorWrapper(GError *err)
{
    mErr = err;
}

GerrorWrapper::~GerrorWrapper()
{
    if (nullptr != mErr) {
        g_error_free (mErr);
    }
}

int GerrorWrapper::code()
{
    if (!mErr) {
        return -1;
    }

    return mErr->code;
}

QString GerrorWrapper::domain()
{
    if (nullptr != mErr) {
        return g_quark_to_string(mErr->domain);
    }
    return nullptr;
}

QString GerrorWrapper::message()
{
    if (nullptr != mErr) {
        return mErr->message;
    }

    return nullptr;
}

std::shared_ptr<GerrorWrapper> GerrorWrapper::wrapFrom(GError *err)
{
    return std::make_shared<GerrorWrapper>(err);
}
