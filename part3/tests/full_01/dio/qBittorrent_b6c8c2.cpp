void PreviewSelect::on_previewButton_clicked() {
  const QModelIndexList selectedIndexes = previewList->selectionModel()->selectedRows(NAME);
  if (selectedIndexes.isEmpty()) return;
  
  // Flush data
  h.flush_cache();

  const QModelIndex &index = selectedIndexes.first();
  const QString path = h.absolute_files_path().at(indexes.at(index.row()));
  
  if (QFile::exists(path)) {
    emit readyToPreviewFile(path);
  } else {
    qDebug("Cannot find file: %s", path.toLocal8Bit().data());
    QMessageBox::critical(0, tr("Preview impossible"), tr("Sorry, we can't preview this file"));
  }
  close();
}