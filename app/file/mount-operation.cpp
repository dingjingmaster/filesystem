#include "mount-operation.h"

#include <QDialog>
#include <QMessageBox>
#include <QPushButton>

#include <gobject/gerror-wrapper.h>

#include <dialog/connect-remote-filesystem.h>


MountOperation::MountOperation(QString uri, QObject *parent) : QObject (parent)
{
    mVolume = g_file_new_for_uri(uri.toUtf8().constData());
    mOp = g_mount_operation_new();
    mCancellable = g_cancellable_new();
}

MountOperation::~MountOperation()
{
    disconnect();
    //g_object_disconnect (G_OBJECT (mOp), "any_signal::signal_name", nullptr);
    g_signal_handlers_disconnect_by_func(mOp, (void *)G_CALLBACK(askPasswordCB), nullptr);
    g_signal_handlers_disconnect_by_func(mOp, (void *)G_CALLBACK(askQuestionCB), nullptr);
    g_signal_handlers_disconnect_by_func(mOp, (void *)G_CALLBACK(abortedCB), nullptr);
    g_object_unref(mVolume);
    g_object_unref(mOp);
    g_object_unref(mCancellable);
}

void MountOperation::setAutoDelete(bool isAuto)
{
    mAutoDelete = isAuto;
}

void MountOperation::slotStart()
{
    ConnectRemoteFilesystem *dlg = new ConnectRemoteFilesystem;
    connect(dlg, &QDialog::accepted, [=]() {
        g_mount_operation_set_username(mOp, dlg->user().toUtf8().constData());
        g_mount_operation_set_password(mOp, dlg->password().toUtf8().constData());
//        g_mount_operation_set_domain(mOp, dlg->domain().toUtf8().constData());
//        g_mount_operation_set_anonymous(mOp, false);
        if (nullptr != mVolume) {
            g_object_unref(mVolume);
        }
        remoteUri = dlg->uri();
        mVolume = g_file_new_for_uri(dlg->uri().toUtf8().constData());
        //TODO: when FileEnumerator::prepare(), trying mount volume without password dialog first.
        g_mount_operation_set_password_save(mOp, G_PASSWORD_SAVE_FOR_SESSION);
    });

    auto code = dlg->exec();
    if (code == QDialog::Rejected) {
        slotCancel();
        QMessageBox msg;
        msg.setText(tr("Operation Cancelled"));
        msg.exec();
        return;
    }
    dlg->deleteLater();
    g_file_mount_enclosing_volume(mVolume, G_MOUNT_MOUNT_NONE, mOp, mCancellable, GAsyncReadyCallback(mountEnclosingVolumeCallback), this);

    g_signal_connect (mOp, "ask-question", G_CALLBACK(askQuestionCB), this);
    g_signal_connect (mOp, "ask-password", G_CALLBACK (askPasswordCB), this);
    g_signal_connect (mOp, "aborted", G_CALLBACK (abortedCB), this);
}

void MountOperation::slotCancel()
{
    g_cancellable_cancel(mCancellable);
    g_object_unref(mCancellable);
    mCancellable = g_cancellable_new();
    Q_EMIT cancelled();
    if (mAutoDelete) {
        deleteLater();
    }
}

void MountOperation::abortedCB(GMountOperation *op, MountOperation *pThis)
{
    g_mount_operation_reply(op, G_MOUNT_OPERATION_ABORTED);
    pThis->disconnect();
    pThis->deleteLater();
}

void MountOperation::askQuestionCB(GMountOperation *op, char *message, char **choices, MountOperation *pThis)
{
    Q_UNUSED(pThis);
    QMessageBox *msg_box = new QMessageBox;
    msg_box->setText(message);
    char **choice = choices;
    int i = 0;
    while (*choice) {
        QPushButton *button = msg_box->addButton(QString(*choice), QMessageBox::ActionRole);
        connect(button, &QPushButton::clicked, [=]() {
            g_mount_operation_set_choice(op, i);
        });
        *choice++;
        i++;
    }
    //block ui
    msg_box->exec();
    msg_box->deleteLater();
    g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
}

GAsyncReadyCallback MountOperation::mountEnclosingVolumeCallback(GFile *volume, GAsyncResult *res, MountOperation *pThis)
{
    GError *err = nullptr;
    g_file_mount_enclosing_volume_finish (volume, res, &err);

    if ((nullptr == err) || (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_ALREADY_MOUNTED))) {
        Q_EMIT pThis->mountSuccess(pThis->remoteUri);
    } else {
        auto errWarpper = GerrorWrapper::wrapFrom(err);
        pThis->finished(errWarpper);
    }

    pThis->finished(nullptr);
    if (pThis->mAutoDelete) {
        pThis->disconnect();
        pThis->deleteLater();
    }
    return nullptr;
}

void MountOperation::askPasswordCB(GMountOperation *op, const char *message, const char *defaultUser, const char *defaultDomain, GAskPasswordFlags flags, MountOperation *pThis)
{
    Q_UNUSED(message);
    Q_UNUSED(defaultUser);
    Q_UNUSED(defaultDomain);
    Q_UNUSED(flags);
    Q_UNUSED(pThis);

    g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
}
