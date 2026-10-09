# Validation: OMNeT++ port vs. ns-3

Every protocol configuration of `simulations/omnetpp.ini` was run in OMNeT++ 6.4 (Cmdenv) and compared with the ns-3 results of the thesis (`results/summary_all.csv`) by `simulations/validate.py`.
FND, HND and LND are identical in every run; PDR is equal to the precision stored in the summary.
(The security scenarios are not part of the OMNeT++ version for now.)

```

----------------------------- ORIGINAL: each protocol in its own paper settings ------------------------------
run                                             FND omnet/ns3             HND             LND        PDR  
leach_ORIGINAL                                      1035/1035       1152/1152       1327/1327   0.986211  OK
heed_ORIGINAL                                         734/734       1384/1384       2259/2259   0.997956  OK
pegasis_ORIGINAL                                    1616/1616       2017/2017       2186/2186   0.988595  OK
shleach_ORIGINAL                                      484/484         583/583         689/689   0.996585  OK
hleach_ORIGINAL                                     1078/1078       1157/1157       1207/1207   0.950353  OK
eechheed_ORIGINAL                                   1121/1121       1954/1954       3783/3783   0.997831  OK

------------------------------- EDITED: unified environment (fair comparison) --------------------------------
run                                             FND omnet/ns3             HND             LND        PDR  
leach_EDITED                                        1383/1383       1586/1586       1842/1842   0.993952  OK
heed_EDITED                                           658/658       1197/1197       1875/1875   0.996512  OK
heed_fairness_EDITED                                  738/738       1157/1157       1969/1969   0.996732  OK
pegasis_EDITED                                      1324/1324       2352/2352       3504/3504   0.994186  OK
shleach_EDITED                                        647/647         702/702         721/721   0.985844  OK
hleach_EDITED                                       1384/1384       1419/1419       1434/1434   0.992968  OK
eechheed_EDITED                                     1277/1277       1384/1384       1543/1543   0.994936  OK

------------------------------------ IMPROVED: the hybrids after our fix -------------------------------------
run                                             FND omnet/ns3             HND             LND        PDR  
shleach_IMPROVED                                    1480/1480       1542/1542       1558/1558   0.992063  OK
hleach_IMPROVED                                     1473/1473       1509/1509       1525/1525   0.993617  OK
eechheed_IMPROVED                                   1335/1335       1384/1384       1766/1766   0.996207  OK

------------------------------------------ PROPOSED: v1 -> v8-Chain ------------------------------------------
run                                             FND omnet/ns3             HND             LND        PDR  
v1_heed_election_leach_join                           320/320         529/529       1218/1218   0.997623  OK
v2_fairness_penalty                                   334/334         528/528       1285/1285   0.997665  OK
v3_single_pass_reuse_int5                           1645/1645       2001/2001       2203/2203   0.983494  OK
v4_reuse_int15                                      1513/1513       2158/2158       2416/2416   0.970722  OK
v5_backup_int15                                     1513/1513       2116/2116       2356/2356   0.984351  OK
v5x_backup_repair_int5                              1645/1645       1996/1996       2081/2081   0.989505  OK
v5x_backup_repair_int10                             1540/1540       2090/2090       2259/2259   0.987008  OK
v5b_energy_aware_repair                             1645/1645       1996/1996       2116/2116   0.986194  OK
v6_chain_center                                     1521/1521       1913/1913       2025/2025   0.985123  OK
v6_chain_farBS                                      1481/1481       1816/1816       1961/1961   0.979452  OK
v7_chain_backup_center                              1521/1521       1906/1906       2011/2011   0.989135  OK
v7_chain_backup_farBS                               1481/1481       1796/1796       1911/1911   0.988040  OK
v7_1_multihop_int5                                  1466/1466       1896/1896       2011/2011   0.990826  OK
v7_2_multihop_int10                                 1431/1431       1996/1996       2181/2181   0.984303  OK
v8_center                                           2446/2446       2536/2536       2566/2566   0.997208  OK
v8_farBS                                            1406/1406       1551/1551       1606/1606   0.993787  OK
v8_chain_center                                     2446/2446       2536/2536       2566/2566   0.997168  OK
v8_chain_farBS                                      1626/1626       1821/1821       1881/1881   0.995637  OK

----------------------------------------- PROPOSED: side experiment ------------------------------------------
run                                             FND omnet/ns3             HND             LND        PDR  
v7_backup_only_noChain_int10                        1540/1540       2090/2090       2291/2291   0.985763  OK

------------------------------------------ FAR BS: BS at (50,-100) -------------------------------------------
run                                             FND omnet/ns3             HND             LND        PDR  
leach_EDITED_farBS                                    988/988       1230/1230       1652/1652   0.994295  OK
heed_EDITED_farBS                                     350/350         765/765       1375/1375   0.995336  OK
heed_fairness_EDITED_farBS                            407/407         756/756       1386/1386   0.994919  OK
pegasis_EDITED_farBS                                1374/1374       2176/2176       2713/2713   0.990811  OK
shleach_EDITED_farBS                                  390/390         601/601         861/861   0.996734  OK
hleach_EDITED_farBS                                 1121/1121       1156/1156       1174/1174   0.980272  OK
eechheed_EDITED_farBS                                 970/970       1377/1377       1400/1400   0.995210  OK
shleach_IMPROVED_farBS                              1388/1388       1435/1435       1455/1455   0.988878  OK
hleach_IMPROVED_farBS                               1219/1219       1262/1262       1278/1278   0.991184  OK
eechheed_IMPROVED_farBS                             1410/1410       1437/1437       1457/1457   0.991819  OK
v3_single_pass_reuse_int5_farBS                     1161/1161       1546/1546       2902/2902   0.980928  OK
v5b_energy_aware_repair_farBS                       1161/1161       1536/1536       1905/1905   0.987968  OK

------------------------------------ ABLATION: v8 without one improvement ------------------------------------
run                                             FND omnet/ns3             HND             LND        PDR  
v8_without_I1                                       2451/2451       2541/2541       2581/2581   0.997409  OK
v8_without_I2                                       2430/2430       2535/2535       2581/2581   0.996309  OK
v8_without_I3                                       2446/2446       2536/2536       2566/2566   0.997105  OK
v8_without_I4                                       1766/1766       1916/1916       1981/1981   0.995359  OK
v8_without_I5                                       2171/2171       2481/2481       2551/2551   0.997721  OK

-------------------------------------- ROUTING: v8-Chain routing modes ---------------------------------------
run                                             FND omnet/ns3             HND             LND        PDR  
center_mode0                                        2446/2446       2536/2536       2566/2566   0.997208  OK
center_mode1                                        2381/2381       2486/2486       2521/2521   0.996779  OK
center_mode2                                        2446/2446       2536/2536       2566/2566   0.997168  OK
farBS_mode0                                         1406/1406       1551/1551       1606/1606   0.993787  OK
farBS_mode1                                         1586/1586       1801/1801       1866/1866   0.995802  OK
farBS_mode2                                         1626/1626       1821/1821       1881/1881   0.995637  OK

------------------------------------------ ROBUSTNESS: 8 topologies ------------------------------------------
run                                             FND omnet/ns3             HND             LND        PDR  
v5b_center_seed1                                    1743/1743       2031/2031       2121/2121   0.986778  OK
v5b_center_seed7                                    1725/1725       2066/2066       2303/2303   0.976919  OK
v5b_center_seed42                                   1551/1551       2051/2051       2401/2401   0.982669  OK
v5b_center_seed99                                   1681/1681       2051/2051       2201/2201   0.981511  OK
v5b_center_seed555                                  1486/1486       2041/2041       2502/2502   0.982345  OK
v5b_center_seed2024                                 1606/1606       2011/2011       2116/2116   0.986460  OK
v5b_center_seed12345                                1645/1645       1996/1996       2116/2116   0.986194  OK
v5b_center_seed31337                                1410/1410       2041/2041       2141/2141   0.987119  OK
v8_center_seed1                                     2481/2481       2551/2551       2596/2596   0.997520  OK
v8_center_seed7                                     2476/2476       2581/2581       2621/2621   0.997118  OK
v8_center_seed42                                    2416/2416       2546/2546       2581/2581   0.997119  OK
v8_center_seed99                                    2446/2446       2541/2541       2571/2571   0.996922  OK
v8_center_seed555                                   2451/2451       2526/2526       2561/2561   0.997042  OK
v8_center_seed2024                                  2401/2401       2507/2507       2541/2541   0.997033  OK
v8_center_seed12345                                 2446/2446       2536/2536       2566/2566   0.997208  OK
v8_center_seed31337                                 2466/2466       2567/2567       2611/2611   0.997024  OK
v8_farBS_seed1                                      1386/1386       1636/1636       1671/1671   0.993522  OK
v8_farBS_seed7                                      1476/1476       1621/1621       1671/1671   0.994442  OK
v8_farBS_seed42                                     1436/1436       1631/1631       1666/1666   0.993628  OK
v8_farBS_seed99                                     1416/1416       1626/1626       1666/1666   0.993002  OK
v8_farBS_seed555                                    1396/1396       1566/1566       1616/1616   0.993724  OK
v8_farBS_seed2024                                   1346/1346       1571/1571       1631/1631   0.994742  OK
v8_farBS_seed12345                                  1406/1406       1551/1551       1606/1606   0.993787  OK
v8_farBS_seed31337                                  1436/1436       1626/1626       1676/1676   0.992915  OK
v8_chain_center_seed1                               2481/2481       2551/2551       2596/2596   0.997438  OK
v8_chain_center_seed7                               2476/2476       2581/2581       2621/2621   0.996897  OK
v8_chain_center_seed42                              2416/2416       2546/2546       2581/2581   0.997210  OK
v8_chain_center_seed99                              2446/2446       2541/2541       2571/2571   0.997079  OK
v8_chain_center_seed555                             2451/2451       2526/2526       2566/2566   0.996856  OK
v8_chain_center_seed2024                            2401/2401       2507/2507       2541/2541   0.997081  OK
v8_chain_center_seed12345                           2446/2446       2536/2536       2566/2566   0.997168  OK
v8_chain_center_seed31337                           2466/2466       2567/2567       2611/2611   0.997048  OK
v8_chain_farBS_seed1                                1636/1636       1866/1866       1911/1911   0.994081  OK
v8_chain_farBS_seed7                                1646/1646       1871/1871       1916/1916   0.994856  OK
v8_chain_farBS_seed42                               1681/1681       1871/1871       1921/1921   0.993602  OK
v8_chain_farBS_seed99                               1676/1676       1851/1851       1901/1901   0.994335  OK
v8_chain_farBS_seed555                              1671/1671       1831/1831       1876/1876   0.995833  OK
v8_chain_farBS_seed2024                             1596/1596       1826/1826       1876/1876   0.995765  OK
v8_chain_farBS_seed12345                            1626/1626       1821/1821       1881/1881   0.995637  OK
v8_chain_farBS_seed31337                            1681/1681       1861/1861       1901/1901   0.992752  OK

98 runs identical to ns-3, 0 different, 0 without reference
```
