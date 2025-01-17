# Define Wi-Fi details
$wifiName = "-----------"
$wifiPassword = "************"

# Function to connect to Wi-Fi
Function Connect-WiFi {
    Write-Output "Connecting to Wi-Fi: $wifiName"
    netsh wlan delete profile name=$wifiName 2>$null  # Remove old profiles if they exist
    netsh wlan add profile filename=(
        @"
<?xml version="1.0"?>
<WLANProfile xmlns="http://www.microsoft.com/networking/WLAN/profile/v1">
  <name>$wifiName</name>
  <SSIDConfig>
    <SSID>
      <name>$wifiName</name>
    </SSID>
  </SSIDConfig>
  <connectionType>ESS</connectionType>
  <connectionMode>auto</connectionMode>
  <MSM>
    <security>
      <authEncryption>
        <authentication>WPA2PSK</authentication>
        <encryption>AES</encryption>
        <useOneX>false</useOneX>
      </authEncryption>
      <sharedKey>
        <keyType>passPhrase</keyType>
        <protected>false</protected>
        <keyMaterial>$wifiPassword</keyMaterial>
      </sharedKey>
    </security>
  </MSM>
</WLANProfile>
"@ | Out-File -FilePath "$env:temp\WiFiProfile.xml" -Encoding UTF8; "$env:temp\WiFiProfile.xml"
    )
    netsh wlan connect name=$wifiName
}

# Function to check if YouTube is already open
Function Is-YouTubeOpen {
    $processes = Get-Process -ErrorAction SilentlyContinue | Where-Object {
        $_.MainWindowTitle -like "*youtube.com*"
    }
    return $processes.Count -gt 0
}

# Function to open YouTube if not already open
Function Open-YouTube {
    if (-not (Is-YouTubeOpen)) {
        Write-Output "Opening YouTube..."
        Start-Process "https://www.youtube.com"
    } else {
        Write-Output "YouTube is already open."
    }
}

# Main Logic
Connect-WiFi
Open-YouTube

# Schedule script to rerun every 15 minutes
$scriptPath = $MyInvocation.MyCommand.Path
$taskName = "WiFiAndYouTubeChecker"

# Remove any existing task
schtasks /Delete /TN $taskName /F 2>$null

# Create a new scheduled task
schtasks /Create /SC MINUTE /MO 15 /TN $taskName /TR "powershell -WindowStyle Hidden -File `"$scriptPath`"" /F

Write-Output "Script scheduled to run every 15 minutes in the background."