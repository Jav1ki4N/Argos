#pragma once
#include <string>

static const std::string CAPTIVE_PORTAL_HTML = R"raw(<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Argos Configuration</title>
    <style>
      body {
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        background-color: #f4f4f9; 
        color: #333;
        margin: 0;
        padding: 0;
        display: flex;
        justify-content: center;
        align-items: center;
        min-height: 100vh;
      }
      .container {
        background: #ffffff;
        width: 85%;
        max-width: 400px; 
        padding: 2rem 1.5rem;
        border-radius: 12px;
        box-shadow: 0 4px 15px rgba(0,0,0,0.05); 
        box-sizing: border-box;
      }
      h1 {
        text-align: center;
        font-size: 1.5rem;
        margin-top: 0;
        margin-bottom: 1.5rem;
        color: #2c3e50;
      }
      .form-group {
        margin-bottom: 1.2rem;
      }
      label {
        display: block;
        margin-bottom: 0.5rem;
        font-weight: 600;
        font-size: 0.95rem;
        color: #555;
      }
      input[type="text"], input[type="password"] {
        width: 100%;
        padding: 0.85rem;
        border: 1px solid #ddd;
        border-radius: 8px;
        font-size: 1rem; 
        box-sizing: border-box;
        background-color: #fafafa;
        transition: border-color 0.2s;
      }
      input[type="text"]:focus, input[type="password"]:focus {
        border-color: #3498db;
        outline: none;
        background-color: #fff;
        box-shadow: 0 0 0 3px rgba(52, 152, 219, 0.15);
      }
      button {
        width: 100%;
        padding: 1rem;
        background-color: #3498db;
        color: white;
        border: none;
        border-radius: 8px;
        font-size: 1.1rem;
        font-weight: bold;
        cursor: pointer;
        margin-top: 1rem;
        transition: background-color 0.2s;
      }
      button:hover {
        background-color: #2980b9;
      }
      .footer {
        text-align: center;
        margin-top: 2rem;
        font-size: 0.8rem;
        color: #aaa;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>Argos Configuration</h1>
      <form>
        <div class="form-group">
          <label for="ssid">WiFi Name (SSID)</label>
          <input type="text" id="ssid" placeholder="Enter the router name" required>
        </div>
        <div class="form-group">
          <label for="password">WiFi Password</label>
          <input type="password" id="password" placeholder="Enter the WiFi password (if any)">
        </div>
        <div class="form-group">
          <label for="target_ip">Target PC's IP Address</label>
          <input type="text" id="target_ip" placeholder="e.g.: 192.168.1.100" required>
        </div>
        <button type="button" onclick="alert('Hello, World!')">Save and Connect</button>
      </form>
      <div class="footer">Argos ESP32 Captive Portal</div>
    </div>
  </body>
</html>)raw";