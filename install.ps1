$code = {
    Add-PartitionAccessPath -DiskNumber 2 -PartitionNumber 1 -AccessPath F:
    Remove-Item "F:\EFI\BOOT\BOOTX64.EFI"
    Copy-Item "I:\Code\gubernatrix\BOOTX64.EFI" -Destination "F:\EFI\BOOT\BOOTX64.EFI"
    Remove-PartitionAccessPath -DiskNumber 2 -PartitionNumber 1 -AccessPath F:
}

Start-Process -FilePath powershell.exe -ArgumentList "-WindowStyle hidden",$code -verb RunAs -WorkingDirectory C: