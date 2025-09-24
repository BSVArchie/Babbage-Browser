$body = @{
    recipientAddress = "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"
    amount = 1000
    feeRate = 5
} | ConvertTo-Json

Invoke-RestMethod -Uri "http://localhost:8080/transaction/create" -Method POST -ContentType "application/json" -Body $body
