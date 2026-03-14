void Utils::Misc::openFolderSelect(const QString &absolutePath)
{
    const QString path = Utils::Fs::fromNativePath(absolutePath);
    // If the item to select doesn't exist, try to open its parent
    if (!QFileInfo(path).exists()) {
        openPath(path.left(path.lastIndexOf("/")));
        return;
    }
#ifdef Q_OS_WIN
    HRESULT hresult = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    PIDLIST_ABSOLUTE pidl = ::ILCreateFromPathW(reinterpret_cast<PCTSTR>(Utils::Fs::toNativePath(path).utf16()));
    if (pidl) {
        ::SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
        ::ILFree(pidl);
    }
    if ((hresult == S_OK) || (hresult == S_FALSE))
        ::CoUninitialize();
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    static QString defaultFileManager;
    if (defaultFileManager.isEmpty()) {
        QProcess proc;
        proc.start("xdg-mime", QStringList() << "query" << "default" << "inode/directory");
        proc.waitForFinished();
        defaultFileManager = proc.readLine().simplified();
    }
    
    if ((defaultFileManager == "dolphin.desktop") || (defaultFileManager == "org.kde.dolphin.desktop"))
        QProcess::startDetached("dolphin", QStringList() << "--select" << Utils::Fs::toNativePath(path));
    else if ((defaultFileManager == "nautilus.desktop") || (defaultFileManager == "org.gnome.Nautilus.desktop")
             || (defaultFileManager == "nautilus-folder-handler.desktop"))
        QProcess::startDetached("nautilus", QStringList() << "--no-desktop" << Utils::Fs::toNativePath(path));
    else if (defaultFileManager == "nemo.desktop")
        QProcess::startDetached("nemo", QStringList() << "--no-desktop" << Utils::Fs::toNativePath(path));
    else if ((defaultFileManager == "konqueror.desktop") || (defaultFileManager == "kfmclient_dir.desktop"))
        QProcess::startDetached("konqueror", QStringList() << "--select" << Utils::Fs::toNativePath(path));
    else
        // "caja" manager can't pinpoint the file, see: https://github.com/qbittorrent/qBittorrent/issues/5003
        openPath(path.left(path.lastIndexOf("/")));
#else
    openPath(path.left(path.lastIndexOf("/")));
#endif
}