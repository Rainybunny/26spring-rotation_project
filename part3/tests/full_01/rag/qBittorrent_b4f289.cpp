// Open the parent directory of the given path with a file manager and select
// (if possible) the item at the given path
void Utils::Misc::openFolderSelect(const QString &absolutePath)
{
    const QString path = Utils::Fs::fromNativePath(absolutePath);
    // If the item to select doesn't exist, try to open its parent
    if (!QFileInfo(path).exists()) {
        openPath(path.left(path.lastIndexOf('/')));
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
    static QString fileManager;
    if (fileManager.isEmpty()) {
        QProcess proc;
        proc.start("xdg-mime", QStringList() << "query" << "default" << "inode/directory");
        proc.waitForFinished();
        fileManager = proc.readLine().simplified();
    }
    
    if ((fileManager == "dolphin.desktop") || (fileManager == "org.kde.dolphin.desktop"))
        QProcess::startDetached("dolphin", QStringList() << "--select" << Utils::Fs::toNativePath(path));
    else if ((fileManager == "nautilus.desktop") || (fileManager == "org.gnome.Nautilus.desktop")
             || (fileManager == "nautilus-folder-handler.desktop"))
        QProcess::startDetached("nautilus", QStringList() << "--no-desktop" << Utils::Fs::toNativePath(path));
    else if (fileManager == "nemo.desktop")
        QProcess::startDetached("nemo", QStringList() << "--no-desktop" << Utils::Fs::toNativePath(path));
    else if ((fileManager == "konqueror.desktop") || (fileManager == "kfmclient_dir.desktop"))
        QProcess::startDetached("konqueror", QStringList() << "--select" << Utils::Fs::toNativePath(path));
    else
        // "caja" manager can't pinpoint the file, see: https://github.com/qbittorrent/qBittorrent/issues/5003
        openPath(path.left(path.lastIndexOf('/')));
#else
    openPath(path.left(path.lastIndexOf('/')));
#endif
}