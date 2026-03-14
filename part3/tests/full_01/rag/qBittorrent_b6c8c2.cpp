void PreviewSelect::on_previewButton_clicked() {
    static const QString errorTitle = tr("Preview impossible");
    static const QString errorMsg = tr("Sorry, we can't preview this file");

    QModelIndexList selectedIndexes = previewList->selectionModel()->selectedRows(NAME);
    if (selectedIndexes.isEmpty())
        return;

    // Flush data
    h.flush_cache();

    const QModelIndex& index = selectedIndexes.first();
    QString path = h.absolute_files_path().at(indexes.at(index.row()));

    if (QFile::exists(path)) {
        emit readyToPreviewFile(path);
    } else {
        qDebug("Cannot find file: %s", path.toLocal8Bit().data());
        QMessageBox::critical(0, errorTitle, errorMsg);
    }

    close();
}